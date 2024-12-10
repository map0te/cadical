#include "internal.hpp"

namespace CaDiCaL {

// As observed by Chanseok Oh and implemented in MapleSAT solvers too,
// various mostly satisfiable instances benefit from long quiet phases
// with less or almost no restarts.  We implement this idea by prohibiting
// the Glucose style restart scheme in a geometric fashion, which is very
// similar to how originally restarts were scheduled in MiniSAT and earlier
// solvers.  We start with say 1e3 = 1000 (opts.stabilizeinit) conflicts of
// Glucose restarts.  Then in a "stabilizing" phase we disable these
// until 1e4 = 2000 conflicts (if 'opts.stabilizefactor' is '200' percent)
// have passed. After that we switch back to regular Glucose style restarts
// until again 2 times more conflicts than the previous limit are reached.
// Actually, in the latest version we still restarts during stabilization
// but only in a reluctant doubling scheme with a rather high interval.

bool Internal::stabilizing () {
  if (!opts.stabilize)
    return false;
  if (stable && opts.stabilizeonly)
    return true;
  if (stats.conflicts >= lim.stabilize) {
    report (stable ? ']' : '}');
    if (stable)
      STOP (stable);
    else
      STOP (unstable);
    stable = !stable;
    if (stable)
      stats.stabphases++;
    PHASE ("stabilizing", stats.stabphases,
           "reached stabilization limit %" PRId64 " after %" PRId64
           " conflicts",
           lim.stabilize, stats.conflicts);
    inc.stabilize *= opts.stabilizefactor * 1e-2;
    if (inc.stabilize > opts.stabilizemaxint)
      inc.stabilize = opts.stabilizemaxint;
    lim.stabilize = stats.conflicts + inc.stabilize;
    if (lim.stabilize <= stats.conflicts)
      lim.stabilize = stats.conflicts + 1;
    swap_averages ();
    PHASE ("stabilizing", stats.stabphases,
           "new stabilization limit %" PRId64
           " at conflicts interval %" PRId64 "",
           lim.stabilize, inc.stabilize);
    report (stable ? '[' : '{');
    if (stable) {
      START (stable);
    }
    else
      START (unstable);
  }
  return stable;
}

// Restarts are scheduled by a variant of the Glucose scheme as presented in
// our POS'15 paper using exponential moving averages.  There is a slow
// moving average of the average recent glucose level of learned clauses as
// well as a fast moving average of those glues.  If the end of a base
// restart conflict interval has passed and the fast moving average is above
// a certain margin over the slow moving average then we restart.

bool Internal::restarting () {
  if (!opts.restart)
    return false;
  if ((size_t) level < assumptions.size () + 2)
    return false;
  if (stabilizing ())
    return reluctant;
  if (stats.conflicts <= lim.restart)
    return false;
  double f = averages.current.glue.fast;
  double margin = (100.0 + opts.restartmargin) / 100.0;
  double s = averages.current.glue.slow, l = margin * s;
  LOG ("EMA glue slow %.2f fast %.2f limit %.2f", s, f, l);
  return l <= f;
}

// This is Marijn's reuse trail idea.  Instead of always backtracking to the
// top we figure out which decisions will be made again anyhow and only
// backtrack to the level of the last such decision or to the top if no such
// decision exists top (in which case we do not reuse any level).

int Internal::reuse_trail () {
  const int trivial_decisions =
      assumptions.size ()
      // Plus 1 if the constraint is satisfied via implications of
      // assumptions and a pseudo-decision level was introduced.
      + !control[assumptions.size () + 1].decision;
  if (!opts.restartreusetrail)
    return trivial_decisions;
  int next_decision = next_decision_variable ();
  assert (1 <= next_decision);
  int res = trivial_decisions;
  if (use_scores ()) {
    while (res < level) {
      int decision = control[res + 1].decision;
      if (decision && score_smaller (this) (abs (decision), next_decision))
        break;
      res++;
    }
  } else {
    int64_t limit = bumped (next_decision);
    while (res < level) {
      int decision = control[res + 1].decision;
      if (decision && bumped (decision) < limit)
        break;
      res++;
    }
  }
  int reused = res - trivial_decisions;
  if (reused > 0) {
    stats.reused++;
    stats.reusedlevels += reused;
    if (stable)
      stats.reusedstable++;
  }
  return res;
}

void Internal::restart () {
  // if rephaserl, choose which phase the next
  // restart window will work on based on MAB
  bool reuse = true; // if mab chooses a different phase, cannot reusetrail
  if (opts.rephaserl) {
    if (randflip == 'U' && stats.restarts % 30) {
      // update llr EMA 
      double llr = (stats.conflicts-mab.last.conflicts)
       / (double) (stats.decisions-mab.last.decisions);
      UPDATE_AVERAGE(averages.current.llr, llr);
      // update MAB
      mab.last.conflicts = stats.conflicts;
      mab.last.decisions = stats.decisions;
      Random random (opts.seed);
      random += stats.restarts;
      mab.unstable_update(llr > averages.current.llr.value);
      // decide on next flip/rand phase
      char last_phase = mab.last.phase;
      mab.unstable_decide(random.generate_int());
      reuse = (last_phase == mab.last.phase);
    } else if (originv == 'S' && stable) {
      // update llr EMA
      double llr = (stats.conflicts-mab.last.conflicts)
        / (double) (stats.decisions-mab.last.decisions);
      UPDATE_AVERAGE(averages.current.llr, llr);
      // update MAB
      mab.last.conflicts = stats.conflicts;
      mab.last.decisions = stats.decisions;
      Random random (opts.seed);
      random += stats.restarts;
      bool suc_trail = (sumtrail / (double) numconflicts) > averages.current.trail.rephase.value;
      bool suc_llr = llr > 1.2*averages.current.llr.value;
      mab.stable_update(suc_llr || suc_trail );
      // decide on next orig/inv phase
      char last_phase = mab.last.phase;
      mab.stable_decide(random.generate_int());
      reuse = (last_phase == mab.last.phase);
    }
  }

  START (restart);
  stats.restarts++;
  stats.restartlevels += level;
  if (stable)
    stats.restartstable++;
  LOG ("restart %" PRId64 "", stats.restarts);
  if (reuse)
    backtrack (reuse_trail ());
  else
    backtrack ();

  lim.restart = stats.conflicts + opts.restartint;
  LOG ("new restart limit at %" PRId64 " conflicts", lim.restart);

  report ('R', 2);
  STOP (restart);
}

} // namespace CaDiCaL
