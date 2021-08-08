#ifndef __TEMPCAL_H__
#define __TEMPCAL_H__

#include "task.hpp"

#include <stdint.h>
#include <Particle.h>
#include "product.hpp"
#include "system.hpp"

#define TCAL_RGB_LED_COLOR      SF_TCAL_RGB_LED_COLOR
#define TCAL_RGB_LED_PATTERN    SF_TCAL_RGB_LED_PATTERN
#define TCAL_RGB_LED_PERIOD     SF_TCAL_RGB_LED_PERIOD
#define TCAL_RGB_LED_PRIORITY   SF_TCAL_RGB_LED_PRIORITY

class TemperatureCal : public Task {
    public:
    void init(void);
    STATES_e run(void);
    void exit(void);
    private:
    SpiffsParticleFile dataFile;
    uint32_t collectionPeriod_s;
    uint32_t measurementTime_s;
    uint8_t calibrationCycles;
    system_tick_t startTime;
};

#endif