#ifndef _averages_hpp_INCLUDED
#define _averages_hpp_INCLUDED

#include "ema.hpp" // alphabetically after 'averages.hpp'

namespace CaDiCaL {

struct Averages {

  int64_t swapped;

  struct {

    struct {
      EMA fast; // average fast (small window) moving glucose level
      EMA slow; // average slow (large window) moving glucose level
    } glue;

    struct {
      EMA fast; // average fast (small window) moving trail level
      EMA slow; // average slow (large window) moving trail level
    } trail;

    EMA size;  // average learned clause size
    EMA jump;  // average (potential non-chronological) back-jump level
    EMA level; // average back track level after conflict

    EMA llr; // average phase window learning rate

  } current, saved;

  Averages () : swapped (0) {}
};

} // namespace CaDiCaL

#endif
