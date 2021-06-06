#ifndef __RIDE_HPP__
#define __RIDE_HPP__
#include "task.hpp"

#include "Particle.h"
#include "product.hpp"

#define RIDE_RGB_LED_COLOR    RGB_COLOR_WHITE
#define RIDE_RGB_LED_PATTERN_GPS  LED_PATTERN_BLINK
#define RIDE_RGB_LED_PERIOD_GPS   500
#define RIDE_RGB_LED_PATTERN_NOGPS  LED_PATTERN_SOLID
#define RIDE_RGB_LED_PERIOD_NOGPS   0
#define RIDE_RGB_LED_PRIORITY LED_PRIORITY_IMPORTANT

#define RIDE_GPS_STARTUP_MS 500
#define RIDE_WATER_DETECT_SURF_SESSION_INIT_WINDOW  WATER_DETECT_SURF_SESSION_INIT_WINDOW

#define RIDE_INIT_WATER_TIMEOUT_MS  SURF_SESSION_GET_INTO_WATER_TIMEOUT_MS

class RideInitTask : public Task
{
    public:
    void init(void);
    STATES_e run(void);
    void exit(void);

    private:
    LEDStatus ledStatus;
    bool gpsLocked;
};

class RideTask : public Task
{
    public:
    void init(void);
    STATES_e run(void);
    void exit(void);
    private:
    LEDStatus ledStatus;
    bool gpsLocked;
    system_tick_t startTime;

};


typedef struct DeploymentSchedule_ DeploymentSchedule_t;

/**
 * @brief Ensemble function.
 * 
 * This function executes once to update the ensemble state.  This should update
 * the accumulators with a new measurement.  If the accumulators have 
 * accumulated the proper amount of data, this function should then record the 
 * proper data.
 * 
 * Essentially, this will be called every ensembleInterval ms after 
 * ensembleDelay ms from the start of the deployment.
 */
typedef void (*EnsembleFunction)(DeploymentSchedule_t* pDeployment); 

/**
 * @brief Ensemble initialization function.
 * 
 * This function is executed once when all of the 
 * 
 */
typedef void (*EnsembleInit)(DeploymentSchedule_t* pDeployment);

struct DeploymentSchedule_
{
    const EnsembleFunction func;
    const EnsembleInit init;
    const size_t measurementsToAccumulate;
    const uint32_t ensembleDelay;
    /**
     * @brief Interval between ensembles in ms
     * 
     * Set to UINT32_MAX to execute only once.
     * 
     */
    const uint32_t ensembleInterval;
    size_t lastExecuteTime;
    system_tick_t startTime;

    void* pData;
};
#endif