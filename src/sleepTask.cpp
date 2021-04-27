#include "sleepTask.hpp"

#include "Particle.h"
#include "product.hpp"
#include "system.hpp"
#include "conio.hpp"

SleepTask::BOOT_BEHAVIOR_e SleepTask::bootBehavior;

void SleepTask::init(void)
{
    SF_OSAL_printf("Entering SYSTEM_STATE_DEEP_SLEEP\n");
    this->ledStatus.setColor(SLEEP_RGB_LED_COLOR);
    this->ledStatus.setPattern(SLEEP_RGB_LED_PATTERN);
    this->ledStatus.setPeriod(SLEEP_RGB_LED_PERIOD);
    this->ledStatus.setPriority(SLEEP_RGB_LED_PRIORITY);
    this->ledStatus.setActive();
    if(digitalRead(SF_USB_PWR_DETECT_PIN))
    {
        return;
    }

    if(pSystemDesc->flags->batteryLow)
    {
        this->bootBehavior = BOOT_BEHAVIOR_NORMAL;
    }

    // commit EEPROM before we bring down everything
    pSystemDesc->nvram->put(NVRAM::BOOT_BEHAVIOR, this->bootBehavior);
    pSystemDesc->nvram->put(NVRAM::NVRAM_VALID, true);

    // bring down the system safely
    SYS_deinitSys();

    switch(this->bootBehavior)
    {
        case BOOT_BEHAVIOR_UPLOAD_REATTEMPT:
            if(digitalRead(WKP_PIN) == HIGH)
            {
                System.sleep(SLEEP_MODE_SOFTPOWEROFF);
            }
            else
            {
                SF_OSAL_printf("Waking up in %ld seconds...ZZZzzzzz\n", SF_UPLOAD_REATTEMPT_DELAY_SEC);
                System.sleep(SLEEP_MODE_SOFTPOWEROFF, SF_UPLOAD_REATTEMPT_DELAY_SEC);
            }
        default:
            System.sleep(SLEEP_MODE_SOFTPOWEROFF);
            break;
    }

}

STATES_e SleepTask::run(void)
{
    if(pSystemDesc->flags->hasCharger)
    {
        return STATE_CHARGE;
    }
    // System.reset();
    return STATE_NULL;
}

void SleepTask::exit(void)
{
    return;
}

void SleepTask::loadBootBehavior(void)
{
    uint8_t bootValid = 0;
    if(!pSystemDesc->nvram->get(NVRAM::NVRAM_VALID, bootValid))
    {
        SleepTask::bootBehavior = SleepTask::BOOT_BEHAVIOR_NORMAL;
        return;
    }

    if(bootValid)
    {
        if(!pSystemDesc->nvram->get(NVRAM::BOOT_BEHAVIOR, SleepTask::bootBehavior))
        {
            SleepTask::bootBehavior = SleepTask::BOOT_BEHAVIOR_NORMAL;
            return;
        }
        bootValid = 0;
        if(!pSystemDesc->nvram->put(NVRAM::NVRAM_VALID, bootValid))
        {
            SF_OSAL_printf("Failed to clear boot flag\n");
            return;
        }
    }
    else
    {
        SleepTask::bootBehavior = SleepTask::BOOT_BEHAVIOR_NORMAL;
        return;
    }
}

SleepTask::BOOT_BEHAVIOR_e SleepTask::getBootBehavior(void)
{
    return SleepTask::bootBehavior;
}

void SleepTask::setBootBehavior(SleepTask::BOOT_BEHAVIOR_e behavior)
{
    SleepTask::bootBehavior = behavior;
}