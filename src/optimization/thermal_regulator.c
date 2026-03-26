#include <unistd.h>
#include <stdio.h>
#include "thermal_regulator.h"

void thermal_throttle_check(int load) {
    if (load > 90) {
        usleep(1000); // Throttling dynamique pour protection thermique
    }
}
