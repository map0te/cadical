#ifndef _thompson_hpp_INCLUDED
#define _thompson_hpp_INCLUDED

#include <random>

namespace CaDiCaL {

struct Bandit {
    double alpha, beta; // beta distribution parameters
    std::gamma_distribution<> a, b; // gammas to simulate beta
    double sample(int seed) {
        // use provided seed for reproducibility
        // sample from beta distribution and return value
        std::mt19937 gen(seed);
        double temp = a(gen);
        return temp / (temp + b(gen));
    }
    void update(bool success) {
        // update beta distribution with success
        alpha += success;
        beta += !success;
        a = std::gamma_distribution<> (alpha);
        b = std::gamma_distribution<> (beta);
    }
    Bandit () : alpha (1.0), beta (1.0), a (1.0), b (1.0) {}
};

struct MAB {
    bool reinstanced = true;

    struct {
        char phase = 'F';
        int64_t decisions = 1;
        int64_t conflicts = 0;
    } last;

    // unstable MAB
    Bandit F, R, O, I;
    void unstable_decide(int seed) {
        if (F.sample(seed) > R.sample(seed)) {
            last.phase = 'F';
        } else {
            last.phase = '#';
        }
    }
    void unstable_update(double llr, double ema_llr) {
        switch (last.phase) {
        case 'F':
            F.update(llr > ema_llr);
            break;
        case '#':
            R.update(llr > ema_llr);
            break;
        }
    }
    void stable_decide(int seed) {
        if (O.sample(seed) > I.sample(seed)) {
            last.phase = 'O';
        } else {
            last.phase = 'I';
        }
    }
    void stable_update(double llr, double ema_llr) {
        switch (last.phase) {
        case 'O':
            O.update(llr < ema_llr);
            break;
        case 'I':
            I.update(llr < ema_llr);
            break;
        }
    }
    void reset() {
        F.alpha = 1.0;
        F.beta = 1.0;
        R.alpha = 1.0;
        R.beta = 1.0;
        O.alpha = 1.0;
        O.beta = 1.0;
        I.alpha = 1.0;
        I.beta = 1.0;
        F.a = std::gamma_distribution<> (1.0);
        F.b = std::gamma_distribution<> (1.0);
        R.a = std::gamma_distribution<> (1.0);
        R.b = std::gamma_distribution<> (1.0);
        O.a = std::gamma_distribution<> (1.0);
        O.b = std::gamma_distribution<> (1.0);
        I.a = std::gamma_distribution<> (1.0);
        I.b = std::gamma_distribution<> (1.0);
        last.decisions = 1;
        last.conflicts = 0;
    }
};

} // namespace CaDiCaL

#endif