#include "flog.hpp"

#include <Particle.h>

#include "conio.hpp"
typedef struct FLOG_Entry_
{

    uint32_t timestamp_ms;
    uint16_t errorCode;
    uint16_t param;
}FLOG_Entry_t;

typedef struct FLOG_Data_
{
    uint32_t numEntries;
    uint32_t nNumEntries;
    FLOG_Entry_t flogEntries[FLOG_NUM_ENTRIES];
}FLOG_Data_t;

typedef struct FLOG_Message_
{
    FLOG_CODE_e code;
    const char* message;
}FLOG_Message_t;

retained FLOG_Data_t flogData;
static char FLOG_unknownMessage[256];

static const char* FLOG_FindMessage(FLOG_CODE_e code);
static int FLOG_IsInitialized(void);

const FLOG_Message_t FLOG_Message[] = {
    {FLOG_SYS_START, "System Start"},
    {FLOG_SYS_BADSRAM, "Bad SRAM"},
    {FLOG_SYS_STARTSTATE, "Starting State"},
    {FLOG_SYS_INITSTATE, "Initializing State"},
    {FLOG_SYS_EXECSTATE, "Executing State Body"},
    {FLOG_SYS_EXITSTATE, "Exiting State"},
    {FLOG_CAL_ACTION, "Calibrate Action"},
    {FLOG_CAL_INIT, "Calibrate Initialization"},
    {FLOG_CAL_START_RUN, "Calibrate Start RUN"},
    {FLOG_CAL_LIMIT, "Calibrate Limit of Cycles"},
    {FLOG_CAL_DONE, "Calibration complete"},
    {FLOG_CAL_EXIT, "Calbiration Exit"},
    {FLOG_CAL_SLEEP, "Calibration Sleep"},
    {FLOG_NULL, NULL}
};

void FLOG_Initialize(void)
{
    if(flogData.nNumEntries != ~flogData.numEntries)
    {
        FLOG_ClearLog();
    }
}
void FLOG_AddError(FLOG_CODE_e errorCode, uint16_t parameter)
{
    FLOG_Entry_t* pEntry;

    if(!FLOG_IsInitialized())
    {
        FLOG_Initialize();
    }

    pEntry = &flogData.flogEntries[(flogData.numEntries + 1) & (FLOG_NUM_ENTRIES - 1)];
    pEntry->timestamp_ms = millis();
    pEntry->errorCode = errorCode;
    pEntry->param = parameter;
    flogData.numEntries++;
    flogData.nNumEntries = ~flogData.numEntries;
}

void FLOG_DisplayLog(void)
{
    uint32_t i;
    if(!FLOG_IsInitialized())
    {
        SF_OSAL_printf("Fault Log not initialized!\n");
        return;
    }

    i = 0;
    if(flogData.numEntries > FLOG_NUM_ENTRIES)
    {
        SF_OSAL_printf("Fault Log overrun!\n");
        i = flogData.numEntries - FLOG_NUM_ENTRIES;
    }

    for(; i < flogData.numEntries; i++)
    {
        SF_OSAL_printf("%8d %32s, parameter: 0x%04X\n", 
            flogData.flogEntries[i & (FLOG_NUM_ENTRIES - 1)].timestamp_ms, 
            FLOG_FindMessage((FLOG_CODE_e) flogData.flogEntries[i & (FLOG_NUM_ENTRIES - 1)].errorCode), 
            flogData.flogEntries[i & (FLOG_NUM_ENTRIES - 1)].param);
    }
    SF_OSAL_printf("\n");
}
void FLOG_ClearLog(void)
{
        memset(&flogData, 0, sizeof(FLOG_Data_t));
        flogData.nNumEntries = ~flogData.numEntries;
}

static const char* FLOG_FindMessage(FLOG_CODE_e code)
{
    const FLOG_Message_t* pEntry;
    for(pEntry = FLOG_Message; pEntry->code; pEntry++)
    {
        if(pEntry->code == code)
        {
            return pEntry->message;
        }
    }
    sprintf(FLOG_unknownMessage, "Unknown FLOG Code: 0x%04X", code);
    return FLOG_unknownMessage;
}

static int FLOG_IsInitialized(void)
{
    return flogData.nNumEntries == ~flogData.numEntries;
}