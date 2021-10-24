#include "utils.hpp"

#include "Particle.h"
#include <cstring>

void UTIL_sleepUntil(system_tick_t ticks)
{
    system_tick_t now = millis();
    if(ticks <= now)
    {
        // late!
        return;
    }
    // if(ticks - now < 30000)
    // {
    //     SystemSleepConfiguration sleepConfig;
    //     sleepConfig.mode(SystemSleepMode::ULTRA_LOW_POWER);
    //     sleepConfig.duration(ticks - now);
    //     System.sleep(sleepConfig);
    //     return;
    // }
    while(millis() < ticks)
    {

    }
    return;
}
