#include "ride.hpp"

#include "Particle.h"

#include "conio.hpp"
#include "ensembleTypes.hpp"
#include "product.hpp"
#include "system.hpp"
#include "sleepTask.hpp"
#include "utils.hpp"

static void initializeSchedule(system_tick_t startTime);
static void getNextEvent(DeploymentSchedule_t** pEventPtr, size_t* pNextTime);

static void SS_ensemble10Func(DeploymentSchedule_t* pDeployment);
static void SS_ensemble10Init(DeploymentSchedule_t* pDeployment);

static void SS_ensemble07Func(DeploymentSchedule_t* pDeployment);
static void SS_ensemble07Init(DeploymentSchedule_t* pDeployment);

static void SS_ensemble08Func(DeploymentSchedule_t* pDeployment);
static void SS_ensemble08Init(DeploymentSchedule_t* pDeployment);

typedef struct Ensemble10_eventData_
{
    double temperature;
    int32_t water;
    int32_t acc[3];
    int32_t ang[3];
    int32_t mag[3];
    int32_t location[2];
    uint8_t hasGPS;
    uint32_t accumulateCount;
}Ensemble10_eventData_t;

typedef struct Ensemble07_eventData_
{
    float battVoltage;
    uint32_t accumulateCount;
}Ensemble07_eventData_t;

typedef struct Ensemble08_eventData_
{
    double temperature;
    int32_t water;

    uint32_t accumulateCount;
}Ensemble08_eventData_t;

static Ensemble10_eventData_t ensemble10Data;
static Ensemble07_eventData_t ensemble07Data;
static Ensemble08_eventData_t ensemble08Data;

DeploymentSchedule_t deploymentSchedule[] = 
{
    {&SS_ensemble10Func, &SS_ensemble10Init, 1, 0, 1000, 0, 0, &ensemble10Data},
    {&SS_ensemble07Func, &SS_ensemble07Init, 1, 0, 10000, 0, 0, &ensemble07Data},
    {&SS_ensemble08Func, &SS_ensemble08Init, 1, 0, UINT32_MAX, 0, 0, &ensemble08Data},
    {NULL, NULL, 0, 0, 0, 0, 0, NULL}
};



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
    char depName[REC_SESSION_NAME_MAX_LEN + 1];
    TinyGPSDate gpsDate;
    TinyGPSTime gpsTime;
    system_tick_t initTime_ms = millis();
    uint8_t waterStatus;
    int i;

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
            snprintf(depName, REC_SESSION_NAME_MAX_LEN, "%02d%02d%02d-%02d%02d%02d", gpsDate.year(), gpsDate.month(), gpsDate.day(), gpsTime.hour(), gpsTime.minute(), gpsTime.second());
            pSystemDesc->pRecorder->setSessionName(depName);
            SF_OSAL_printf("Filename is %s\n", depName);
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
        waterStatus = pSystemDesc->pWaterSensor->takeReading();
        if(waterStatus == WATER_SENSOR_HIGH_STATE)
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

void RideTask::init(void)
{
    SF_OSAL_printf("Entering STATE_DEPLOYED\n");
    this->startTime = millis();
    initializeSchedule(this->startTime);
    pSystemDesc->pRecorder->openSession(NULL);

    // initialize sensors
    if(!pSystemDesc->pIMU->open())
    {
        SF_OSAL_printf("IMU Fail\n");
    }
    if(!pSystemDesc->pCompass->open())
    {
        SF_OSAL_printf("Mag Fail\n");
    }
    if(!pSystemDesc->pTempSensor->init())
    {
        SF_OSAL_printf("Temp Fail\n");
    }
}

/**
 * @brief 
 * 
 * @return STATES_e 
 */
STATES_e RideTask::run(void)
{
    DeploymentSchedule_t* pNextEvent = NULL;
    size_t nextEventTime;
    while(1)
    {
        while(GPS_kbhit())
        {
            pSystemDesc->pGPS->encode(GPS_getch());
        }
        
        getNextEvent(&pNextEvent, &nextEventTime);
        while(millis() < nextEventTime)
        {
            continue;
        }
        pNextEvent->func(pNextEvent);
        pNextEvent->lastExecuteTime = nextEventTime;

        if(pSystemDesc->pWaterSensor->takeReading() == WATER_SENSOR_LOW_STATE)
        {
            SF_OSAL_printf("Out of water\n");
            return STATE_UPLOAD;
        }

        if(pSystemDesc->flags->batteryLow)
        {
            SF_OSAL_printf("Low Battery!\n");
            return STATE_DEEP_SLEEP;
        }
    }
    return STATE_DEEP_SLEEP;
}

void RideTask::exit(void)
{
    SF_OSAL_printf("Closing session\n");
    pSystemDesc->pRecorder->closeSession();
    // Deinitialize sensors
    pSystemDesc->pTempSensor->stop();
    pSystemDesc->pCompass->close();
    pSystemDesc->pIMU->close();
    pSystemDesc->pGPS->gpsModuleStop();

}

static void initializeSchedule(system_tick_t startTime)
{
    DeploymentSchedule_t* pDeployment;
    for(pDeployment = deploymentSchedule; pDeployment->init; pDeployment++)
    {
        pDeployment->startTime = startTime;
        pDeployment->lastExecuteTime = 0;
        pDeployment->init(pDeployment);
    }
}

static void getNextEvent(DeploymentSchedule_t ** pEventPtr, size_t* pNextTime)
{
    size_t earliestExecution = 0;
    uint32_t earliestEvent = 0;
    size_t timeToCompare;
    uint32_t i = 0;

    *pEventPtr = 0;
    *pNextTime = 0;

    for(i = 0; deploymentSchedule[i].func; i++)
    {
        if(deploymentSchedule[i].lastExecuteTime == 0)
        {
            timeToCompare = deploymentSchedule[i].startTime + deploymentSchedule[i].ensembleDelay;
        }
        else
        {
            if(deploymentSchedule[i].ensembleInterval != UINT32_MAX)
            {
                timeToCompare = deploymentSchedule[i].lastExecuteTime + deploymentSchedule[i].ensembleInterval;
            }
            else
            {
                continue;
            }
        }
        // SF_OSAL_printf("Event %d nextExecute: %d\n", i, timeToCompare);

        if(earliestExecution == 0)
        {
            earliestExecution = timeToCompare;
        }

        if(timeToCompare < earliestExecution)
        {
            earliestExecution = timeToCompare;
            earliestEvent = i;
        }
    }
    *pNextTime = earliestExecution;
    *pEventPtr = deploymentSchedule + earliestEvent;
}

static void SS_ensemble10Init(DeploymentSchedule_t* pDeployment)
{
    memset(&ensemble10Data, 0, sizeof(Ensemble10_eventData_t));
    pDeployment->pData = &ensemble10Data;
}

static void SS_ensemble07Init(DeploymentSchedule_t* pDeployment)
{
    memset(&ensemble07Data, 0, sizeof(Ensemble07_eventData_t));
    pDeployment->pData = &ensemble07Data;
}

static void SS_ensemble08Init(DeploymentSchedule_t* pDeployment)
{
    memset(&ensemble08Data, 0, sizeof(Ensemble08_eventData_t));
    pDeployment->pData = &ensemble08Data;
}
static void SS_ensemble10Func(DeploymentSchedule_t* pDeployment)
{
    float temp;
    uint8_t water;
    int32_t lat, lng;
    uint16_t accelRawData[3];
    uint16_t gyroRawData[3];
    uint16_t magRawData[3];
    float accelData[3];
    float gyroData[3];
    int16_t magData[3];
    bool hasGPS = false;
    Ensemble10_eventData_t* pData = (Ensemble10_eventData_t*)pDeployment->pData;

    #pragma pack(push, 1)
    struct
    {
        EnsembleHeader_t header;
        union{
            Ensemble10_data_t ens10;
            Ensemble11_data_t ens11;
        }data;
    }ensData;
    #pragma pack(pop)


    // Obtain measurements    
    temp = pSystemDesc->pTempSensor->getTemp();
    water = pSystemDesc->pWaterSensor->getCurrentReading();

    pSystemDesc->pIMU->get_accelerometer(accelData, accelData + 1, accelData + 2);
    pSystemDesc->pIMU->get_accel_raw_data((uint8_t*) accelRawData);

    pSystemDesc->pIMU->get_gyroscope(gyroData, gyroData + 1, gyroData + 2);
    pSystemDesc->pIMU->get_gyro_raw_data((uint8_t*) gyroRawData);
    
    pSystemDesc->pCompass->read(magData, magData + 1, magData + 2);
    pSystemDesc->pCompass->read((uint8_t*) magRawData);

    if(pSystemDesc->pGPS->location.isValid() && pSystemDesc->pGPS->location.isUpdated() && (pSystemDesc->pGPS->location.age() < GPS_AGE_VALID_MS))
    {
        hasGPS = true;
        lat = pSystemDesc->pGPS->location.lat_int32();
        lng = pSystemDesc->pGPS->location.lng_int32();
    }
    else
    {
        hasGPS = false;
        lat = pData->location[0];
        lng = pData->location[1];
    }

    // Accumulate measurements
    pData->temperature += temp;
    pData->water += water;
    pData->acc[0] += B_TO_N_ENDIAN_2(accelRawData[0]);
    pData->acc[1] += B_TO_N_ENDIAN_2(accelRawData[1]);
    pData->acc[2] += B_TO_N_ENDIAN_2(accelRawData[2]);
    pData->ang[0] += B_TO_N_ENDIAN_2(gyroRawData[0]);
    pData->ang[1] += B_TO_N_ENDIAN_2(gyroRawData[1]);
    pData->ang[2] += B_TO_N_ENDIAN_2(gyroRawData[2]);
    pData->mag[0] += B_TO_N_ENDIAN_2(magRawData[0]);
    pData->mag[1] += B_TO_N_ENDIAN_2(magRawData[1]);
    pData->mag[2] += B_TO_N_ENDIAN_2(magRawData[2]);
    pData->location[0] += lat;
    pData->location[1] += lng;
    pData->hasGPS += hasGPS ? 1 : 0;
    pData->accumulateCount++;


    // Report accumulated measurements
    if(pData->accumulateCount == pDeployment->measurementsToAccumulate)
    {
        water = pData->water / pDeployment->measurementsToAccumulate;
        temp = pData->temperature / pDeployment->measurementsToAccumulate;
        if(water == false)
        {
            temp -= 100;
        }

        ensData.header.elapsedTime_ds = ((millis() - pDeployment->startTime) / 100) & 0x00FFFFFF;
        SF_OSAL_printf("Ensemble timestamp: %d\n", ensData.header.elapsedTime_ds);
        ensData.data.ens10.rawTemp = N_TO_B_ENDIAN_2(temp / 0.0078125);
        ensData.data.ens10.rawAcceleration[0] = N_TO_B_ENDIAN_2(pData->acc[0] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawAcceleration[1] = N_TO_B_ENDIAN_2(pData->acc[1] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawAcceleration[2] = N_TO_B_ENDIAN_2(pData->acc[2] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawAngularVel[0] = N_TO_B_ENDIAN_2(pData->ang[0] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawAngularVel[1] = N_TO_B_ENDIAN_2(pData->ang[1] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawAngularVel[2] = N_TO_B_ENDIAN_2(pData->ang[2] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawMagField[0] = N_TO_B_ENDIAN_2(pData->mag[0] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawMagField[1] = N_TO_B_ENDIAN_2(pData->mag[1] / pDeployment->measurementsToAccumulate);
        ensData.data.ens10.rawMagField[2] = N_TO_B_ENDIAN_2(pData->mag[2] / pDeployment->measurementsToAccumulate);
        ensData.data.ens11.location[0] = N_TO_B_ENDIAN_4(pData->location[0] / pDeployment->measurementsToAccumulate);
        ensData.data.ens11.location[1] = N_TO_B_ENDIAN_4(pData->location[1] / pDeployment->measurementsToAccumulate);

        if(pData->hasGPS / pDeployment->measurementsToAccumulate)
        {
            ensData.header.ensembleType = ENS_TEMP_IMU_GPS;
            pSystemDesc->pRecorder->putBytes(&ensData, sizeof(EnsembleHeader_t) + sizeof(Ensemble11_data_t));
        }
        else
        {
            ensData.header.ensembleType = ENS_TEMP_IMU;
            pSystemDesc->pRecorder->putBytes(&ensData, sizeof(EnsembleHeader_t) + sizeof(Ensemble10_data_t));
        }
        
        memset(pData, 0, sizeof(Ensemble10_eventData_t));
    }
}

static void SS_ensemble07Func(DeploymentSchedule_t* pDeployment)
{
    float battVoltage;
    Ensemble07_eventData_t* pData = (Ensemble07_eventData_t*) pDeployment->pData;
    #pragma pack(push, 1)
    struct{
        EnsembleHeader_t header;
        Ensemble07_data_t data;
    }ensData;
    #pragma pack(pop)

    // obtain measurements
    battVoltage = pSystemDesc->battery->getVCell();

    // accumulate measurements
    pData->battVoltage += battVoltage;
    pData->accumulateCount++;

    // Report accumulated measurements
    if(pData->accumulateCount == pDeployment->measurementsToAccumulate)
    {
        ensData.header.elapsedTime_ds = ((millis() - pDeployment->startTime) / 100) & 0x00FFFFFF;
        ensData.header.ensembleType = ENS_BATT;
        ensData.data.batteryVoltage = N_TO_B_ENDIAN_2((pData->battVoltage / pData->accumulateCount) * 1000);

        pSystemDesc->pRecorder->putData(ensData);
        memset(pData, 0, sizeof(Ensemble07_eventData_t));
    }

}

static void SS_ensemble08Func(DeploymentSchedule_t* pDeployment)
{
    float temp;
    uint8_t water;

    Ensemble08_eventData_t* pData = (Ensemble08_eventData_t*) pDeployment->pData;
    #pragma pack(push, 1)
    struct{
        EnsembleHeader_t header;
        Ensemble08_data_t ensData;
    }ens;
    #pragma pack(pop)

    // obtain measurements
    temp = pSystemDesc->pTempSensor->getTemp();
    water = pSystemDesc->pWaterSensor->getCurrentReading();

    // accumulate measurements
    pData->temperature += temp;
    pData->water += water;
    pData->accumulateCount++;

    // Report accumulated measurements
    if(pData->accumulateCount == pDeployment->measurementsToAccumulate)
    {
        water = pData->water / pDeployment->measurementsToAccumulate;
        temp = pData->temperature / pDeployment->measurementsToAccumulate;
        if(water == false)
        {
            temp -= 100;
        }
        
        ens.header.elapsedTime_ds = ((millis() - pDeployment->startTime) / 100) & 0x00FFFFFF;
        ens.header.ensembleType = ENS_TEMP_TIME;
        ens.ensData.rawTemp = N_TO_B_ENDIAN_2(temp / 0.0078125);

        pSystemDesc->pRecorder->putData(ens);
        memset(pData, 0, sizeof(Ensemble08_eventData_t));
    }

}