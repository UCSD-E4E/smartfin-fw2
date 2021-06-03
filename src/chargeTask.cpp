#include "chargeTask.hpp"

#include "Particle.h"
#include "conio.hpp"
#include "system.hpp"
#include "sleepTask.hpp"
#include "consts.h"

static void byteshiftl(void* pData, size_t dataLen, size_t nPos, uint8_t fill);

void ChargeTask::init(void)
{
    SF_OSAL_printf("Entering SYSTEM_STATE_CHARGING\n");
    pSystemDesc->pChargerCheck->start();
    this->ledStatus.setColor(CHARGE_RGB_LED_COLOR);
    this->ledStatus.setPattern(CHARGE_RGB_LED_PATTERN);
    this->ledStatus.setPeriod(CHARGE_RGB_LED_PERIOD);
    this->ledStatus.setPriority(CHARGE_RGB_LED_PRIORITY);
    this->ledStatus.setActive();
    this->startTime = millis();
}

STATES_e ChargeTask::run(void)
{
    const SleepTask::BOOT_BEHAVIOR_e bootBehavior = SleepTask::getBootBehavior();
    while(1)
    {
        if(kbhit())
        {
            this->inputBuffer[CLI_BUFFER_LEN - 1] = getch();
            byteshiftl(this->inputBuffer, CLI_BUFFER_LEN, 1, 0);
            if(strcmp(this->inputBuffer, CLI_INTERRUPT_PHRASE) == 0)
            {
                return STATE_CLI;
            }
        }
        if(!pSystemDesc->flags->hasCharger)
        {
            return STATE_DEEP_SLEEP;
        }
        if(pSystemDesc->pWaterSensor->takeReading())
        {
            return STATE_SESSION_INIT;
        }

        if(bootBehavior == SleepTask::BOOT_BEHAVIOR_UPLOAD_REATTEMPT)
        {
            if(millis() - this->startTime >= SF_UPLOAD_REATTEMPT_DELAY_SEC * MSEC_PER_SEC)
            {
                return STATE_UPLOAD;
            }
        }
        os_thread_yield();
    }
}

void ChargeTask::exit(void)
{
    pSystemDesc->pChargerCheck->stop();
    this->ledStatus.setActive(false);
}

static void byteshiftl(void* pData, size_t dataLen, size_t nPos, uint8_t fill)
{
    uint8_t* pByte = (uint8_t*) pData;
    size_t i;
    for(i = 0; i < dataLen - nPos; i++)
    {
        pByte[i] = pByte[i + nPos];
    }
    for(; i < dataLen; i++)
    {
        pByte[i] = fill;
    }
}