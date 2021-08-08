#include "tempCal.hpp"

#include <Particle.h>

#include "nvram.hpp"
#include "system.hpp"
#include "ride.hpp"
#include "ensembleTypes.hpp"
#include "utils.hpp"


typedef struct Ensemble08_eventData_
{
    double temperature;
    int32_t water;

    uint32_t accumulateCount;
}Ensemble08_eventData_t;

typedef struct Ensemble07_eventData_
{
    float battVoltage;
    uint32_t accumulateCount;
}Ensemble07_eventData_t;

static LEDStatus TCAL_ledStatus;
static Ensemble07_eventData_t ensemble07Data;
static Ensemble08_eventData_t ensemble08Data;

static void SS_ensemble07Func(DeploymentSchedule_t* pDeployment);
static void SS_ensemble07Init(DeploymentSchedule_t* pDeployment);

static void SS_ensemble08Func(DeploymentSchedule_t* pDeployment);
static void SS_ensemble08Init(DeploymentSchedule_t* pDeployment);

static void getNextEvent(DeploymentSchedule_t ** pEventPtr, size_t* pNextTime);
static void initializeSchedule(system_tick_t startTime);

DeploymentSchedule_t calibrateSchedule[] =
{
    {&SS_ensemble07Func, &SS_ensemble07Init, 1, 0, 10000, 0, 0, &ensemble07Data},
    {&SS_ensemble08Func, &SS_ensemble08Init, 1, 0, 1000, 0, 0, &ensemble08Data},
    {NULL, NULL, 0, 0, 0, 0, 0, NULL}
};

void TemperatureCal::init(void)
{
    TCAL_ledStatus.setColor(TCAL_RGB_LED_COLOR);
    TCAL_ledStatus.setPattern(TCAL_RGB_LED_PATTERN);
    TCAL_ledStatus.setPeriod(TCAL_RGB_LED_PERIOD);
    TCAL_ledStatus.setPriority(TCAL_RGB_LED_PRIORITY);
    TCAL_ledStatus.setActive();

    this->dataFile = pSystemDesc->pFileSystem->openFile("TMP116_CAL", SPIFFS_O_RDWR | SPIFFS_O_CREAT | SPIFFS_APPEND);

    pSystemDesc->pTempSensor->init();

    pSystemDesc->pNvram->get(NVRAM::TMP116_CAL_DATA_COLLECTION_PERIOD_SEC, this->measurementTime_s);
    pSystemDesc->pNvram->get(NVRAM::TMP116_CAL_CYCLE_PERIOD_SEC, this->collectionPeriod_s);
    pSystemDesc->pNvram->get(NVRAM::TMP116_CAL_ATTEMPTS_TOTAL, this->calibrationCycles);

    SF_OSAL_printf("Entering STATE_CALIBRATE\n");
    this->startTime = millis();
    initializeSchedule(this->startTime);
}
STATES_e TemperatureCal::run(void)
{
    DeploymentSchedule_t* pNextEvent = NULL;
    size_t nextEventTime;
    getNextEvent(&pNextEvent, &nextEventTime);
    while(millis() < nextEventTime)
    {
        continue;
    }
    pNextEvent->func(pNextEvent);
    pNextEvent->lastExecuteTime = nextEventTime;
    return STATE_DEEP_SLEEP;
}
void TemperatureCal::exit(void)
{
    TCAL_ledStatus.setActive(false);
}

static void getNextEvent(DeploymentSchedule_t ** pEventPtr, size_t* pNextTime)
{
    size_t earliestExecution = 0;
    uint32_t earliestEvent = 0;
    size_t timeToCompare;
    uint32_t i = 0;

    *pEventPtr = 0;
    *pNextTime = 0;

    for(i = 0; calibrateSchedule[i].func; i++)
    {
        if(calibrateSchedule[i].lastExecuteTime == 0)
        {
            timeToCompare = calibrateSchedule[i].startTime + calibrateSchedule[i].ensembleDelay;
        }
        else
        {
            if(calibrateSchedule[i].ensembleInterval != UINT32_MAX)
            {
                timeToCompare = calibrateSchedule[i].lastExecuteTime + calibrateSchedule[i].ensembleInterval;
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
    *pEventPtr = calibrateSchedule + earliestEvent;
}

static void initializeSchedule(system_tick_t startTime)
{
    DeploymentSchedule_t* pDeployment;
    for(pDeployment = calibrateSchedule; pDeployment->init; pDeployment++)
    {
        pDeployment->startTime = startTime;
        pDeployment->lastExecuteTime = 0;
        pDeployment->init(pDeployment);
    }
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
    battVoltage = pSystemDesc->pBattery->getVCell();

    // accumulate measurements
    pData->battVoltage += battVoltage;
    pData->accumulateCount++;

    // Report accumulated measurements
    if(pData->accumulateCount == pDeployment->measurementsToAccumulate)
    {
        ensData.header.elapsedTime_ds = Ens_getStartTime(pDeployment->startTime);
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
        
        ens.header.elapsedTime_ds = Ens_getStartTime(pDeployment->startTime);
        ens.header.ensembleType = ENS_TEMP_TIME;
        ens.ensData.rawTemp = N_TO_B_ENDIAN_2(temp / 0.0078125);

        pSystemDesc->pRecorder->putData(ens);
        memset(pData, 0, sizeof(Ensemble08_eventData_t));
    }

}