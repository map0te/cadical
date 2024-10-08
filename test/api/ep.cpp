#include "../../src/cadical.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

namespace CaDiCaL {

class EP : public CaDiCaL::ExternalPropagator {
    void notify_assignment (int lit, bool is_fixed) {
        printf("Notified %d\n", lit);
    }
    void notify_new_decision_level () {}
    void notify_backtrack (size_t new_level) {}
    bool cb_check_found_model (const std::vector<int> &model) {
        printf("here");
    }
    void notify_restart() {
        printf("Notified restart\n");
    }

    int cb_decide () { return 0; }
    int cb_propagate () { return 0; }
    int cb_add_reason_clause_lit (int propagated_lit) {
        (void) propagated_lit;
        return 0;
    }
    bool cb_restart () { 
        return false;
    }
    bool cb_has_external_clause () { return false; }
    int cb_add_external_clause_lit () { return 0; }

    int restarts=0;
};

}

int main () {
    CaDiCaL::Solver *s = new CaDiCaL::Solver;
    CaDiCaL::ExternalPropagator *ep = new CaDiCaL::EP();
    s->connect_external_propagator(ep);
    s->add_observed_var(1);
    int vars = 0;
    s->read_dimacs ("cnf/add64.cnf", vars);
    //solver->vars();
    int res = s->solve ();
    assert(res==20);
}