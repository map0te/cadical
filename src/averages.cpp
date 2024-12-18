#include "internal.hpp"

namespace CaDiCaL {

void Internal::init_averages () {

  LOG ("initializing averages");

  INIT_EMA (averages.current.jump, opts.emajump);
  INIT_EMA (averages.current.level, opts.emalevel);
  INIT_EMA (averages.current.size, opts.emasize);

  INIT_EMA (averages.current.llr, opts.emaphase);

  INIT_EMA (averages.current.glue.fast, opts.emagluefast);
  INIT_EMA (averages.current.glue.slow, opts.emaglueslow);

  INIT_EMA (averages.current.trail.fast, opts.ematrailfast);
  INIT_EMA (averages.current.trail.slow, opts.ematrailslow);

  assert (!averages.swapped);
}

void Internal::swap_averages () {
  LOG ("saving current averages");
  swap (averages.current, averages.saved);
  if (!averages.swapped)
    init_averages ();
  else
    LOG ("swapping in previously saved averages");
  averages.swapped++;
}

} // namespace CaDiCaL
