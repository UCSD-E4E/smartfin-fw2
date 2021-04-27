#include "ride.hpp"

#include "Particle.h"

#include "conio.hpp"
#include "product.hpp"
#include "system.hpp"
#include "sleepTask.hpp"


void RideInitTask::init(void)
{
    SF_OSAL_printf("Entering SYSTEM_STATE_SURF_SESSION_INIT\n");
    this->ledStatus.setColor(RIDE_RGB_LED_COLOR);
    this->ledStatus.setPattern(RIDE_RGB_LED_PATTERN_NOGPS);
    this->ledStatus.setPeriod(RIDE_RGB_LED_PERIOD_NOGPS);
    this->ledStatus.setPriority(RIDE_RGB_LED_PRIORITY);
    this->ledStatus.setActive();

    digitalWrite(GPS_PWR_EN_PIN, HIGH);
    delay(RIDE_GPS_STARTUP_MS);

    pSystemDesc->pGPS->gpsModuleInit();
    this->gpsLocked = false;
    SF_OSAL_printf("GPS Initialised @ %dms\n", millis());

    // reset no water timeout array to all zeros
    pSystemDesc->pWaterSensor->resetArray();
    // change window to small window (smaller moving average for quick detect)
    pSystemDesc->pWaterSensor->setWindowSize(RIDE_WATER_DETECT_SURF_SESSION_INIT_WINDOW);
    // set initial state to not in water for hysteresis
    pSystemDesc->pWaterSensor->forceState(WATER_SENSOR_LOW_STATE);

}

STATES_e RideInitTask::run(void)
{
    char depName[REC_DEPLOYMENT_NAME_MAX_LEN + 1];
    TinyGPSDate gpsDate;
    TinyGPSTime gpsTime;
    system_tick_t initTime_ms = millis();
    while(1)
    {

        while(GPS_kbhit())
        {
            pSystemDesc->pGPS->encode(GPS_getch());
        }

        gpsDate = pSystemDesc->pGPS->date;
        gpsTime = pSystemDesc->pGPS->time;
        if(gpsDate.value() && gpsTime.value())
        {
            SF_OSAL_printf("GPS Time Recorded @ %dms\n", millis());
            snprintf(depName, REC_DEPLOYMENT_NAME_MAX_LEN, "%02d%02d%02d-%02d%02d%02d", gpsDate.year(), gpsDate.month(), gpsDate.day(), gpsTime.hour(), gpsTime.minute(), gpsTime.second());
            pSystemDesc->pRecorder->setDeploymentName(depName);
        }

        if((pSystemDesc->pGPS->location.age() < GPS_AGE_VALID_MS) && (pSystemDesc->pGPS->location.age() >= 0))
        {
            if(!this->gpsLocked)
            {
                SF_OSAL_printf("GPS Location Lock @ %dms\n", millis());
                this->gpsLocked = true;
            }
            this->ledStatus.setColor(RIDE_RGB_LED_COLOR);
            this->ledStatus.setPattern(RIDE_RGB_LED_PATTERN_NOGPS);
            this->ledStatus.setPeriod(RIDE_RGB_LED_PERIOD_NOGPS);
            this->ledStatus.setPriority(RIDE_RGB_LED_PRIORITY);
            this->ledStatus.setActive();
        }
        else
        {
            this->gpsLocked = false;
            this->ledStatus.setColor(RIDE_RGB_LED_COLOR);
            this->ledStatus.setPattern(RIDE_RGB_LED_PATTERN_GPS);
            this->ledStatus.setPeriod(RIDE_RGB_LED_PERIOD_GPS);
            this->ledStatus.setPriority(RIDE_RGB_LED_PRIORITY);
            this->ledStatus.setActive();
        }

        // if water is detected 
        if(pSystemDesc->pWaterSensor->getCurrentStatus() == WATER_SENSOR_HIGH_STATE)
        {
            return STATE_DEPLOYED;
        }
        else if(millis() > (initTime_ms + RIDE_INIT_WATER_TIMEOUT_MS))
        {
            // water not detected and timeout
            SleepTask::setBootBehavior(SleepTask::BOOT_BEHAVIOR_NORMAL);
            return STATE_DEEP_SLEEP;
        }
        // otherwise continue
        os_thread_yield();
    }
}

void RideInitTask::exit(void)
{
    this->ledStatus.setActive(false);
}