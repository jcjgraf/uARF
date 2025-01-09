#include "spec_lib.h"
#include <stdlib.h>

struct history get_randomized_history(void) {
    return (struct history){
        .hist[0] = random(),
        .hist[1] = random(),
    };
}
