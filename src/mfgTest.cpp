#include "mfgTest.hpp"

#include <Particle.h>
#include <errno.h>
#include <stdint.h>

#include "system.hpp"
#include "conio.hpp"
#include "product.hpp"

typedef int (MfgTest::*mfg_run_function_t)(void);

typedef struct {
    mfg_run_function_t run_function_ptr;
} mfg_test_entry_t;

static bool verify_yes_or_no(const char * prompt_text);
static int led_test(SFLed* pin);

int(* (MfgTest::MFG_TEST_TABLE)[])(void) = {
    &MfgTest::gps_test,
    &MfgTest::imu_test,
    &MfgTest::temperature_sensor_test,
    &MfgTest::cellular_test,
    &MfgTest::wet_dry_sensor_test,
    &MfgTest::status_led_test,
    &MfgTest::battery_status_test,
    &MfgTest::battery_voltage_test,
    NULL
};

void MfgTest::init(void)
{
    pSystemDesc->pGPS->gpsModuleInit();
}

STATES_e MfgTest::run(void)
{
    String deviceID = System.deviceID();
    int retval = 0;

    SF_OSAL_printf("\r\nStarting Manufacturing Testing\r\n");
    SF_OSAL_printf("Testing Device %s\r\n", deviceID.c_str());

    for(uint8_t i = 0; MFG_TEST_TABLE[i]; i++)
    {
        retval = MFG_TEST_TABLE[i]();
        if(retval != 0)
        {
            break;
        }
    }

    if(retval)
    {
        SF_OSAL_printf("Manufacturing Tests FAILED.\r\nMark unit as scrap.\r\n\r\n");
    }
    else
    {
        SF_OSAL_printf("All tests passed.\r\n");
        if (pSystemDesc->pBattery->getSoC() >= 0.80)
        {
            SF_OSAL_printf("Battery level sufficient to proceed to calibration.\r\n\r\n");
        }
        else
        {
            SF_OSAL_printf("HOWEVER BATTERY REQUIRES CHARGE before calibration.\r\n\r\n");
        }
    }
    return STATE_CLI;
}

void MfgTest::exit(void)
{
    pSystemDesc->pGPS->gpsModuleStop();
}

int MfgTest::gps_test(void)
{
    SF_OSAL_printf("Running the GPS Test\r\n");

    if(!pSystemDesc->pGPS->checkComms())
    {
        SF_OSAL_printf("GPS failed\r\n");
        return -ENXIO;
    }
    SF_OSAL_printf("GPS passed\r\n");
    return 0;
}
int MfgTest::imu_test(void)
{
    SF_OSAL_printf("Running the IMU Test\r\n");

    if(!pSystemDesc->pIMU->open())
    {
        SF_OSAL_printf("IMU failed\r\n");
        pSystemDesc->pIMU->close();
        return -ENXIO;
    }
    SF_OSAL_printf("IMU passed\r\n");

    // TODO add Magnetometer

    pSystemDesc->pIMU->close();

    return 0;
}
int MfgTest::temperature_sensor_test(void)
{
    float temp;
    int retval = 0;

    SF_OSAL_printf("Running the Temp Test\r\n");

    pSystemDesc->pTempSensor->init();

    temp = pSystemDesc->pTempSensor->getTemp();

    if ((temp >= MFG_MIN_VALID_TEMPERATURE) && (temp <= MFG_MAX_VALID_TEMPERATURE))
    {
        SF_OSAL_printf("Temp passed: Temp %f\r\n", temp);
    }
    else
    {
        SF_OSAL_printf("Temp failed\r\n");
        retval = -EIO;
    }

    pSystemDesc->pTempSensor->stop();

    return retval;
}
int MfgTest::cellular_test(void)
{
    int retval = 0;
    system_tick_t time_of_init_state_exit_ms = millis();

    SF_OSAL_printf("Running the Cellular Test \r\n");

    // Simply test cellular by getting the time

    SF_OSAL_printf("Please wait to connect to the Particle network.\nWaiting.");
    Particle.connect();

    // while we don't get a valid time and haven't hit the timeout

    while ((Particle.connected() == false) && (millis() < (time_of_init_state_exit_ms + MANUFACTURING_CELL_TIMEOUT_MS)))
    {
        delay(1000);
        SF_OSAL_printf(".");
    }
    
    if (Particle.connected())
    {
        SF_OSAL_printf("\r\nCellular passed\r\n");
    }
    else
    {
        SF_OSAL_printf("\r\nCellular failed\r\n");
        retval = -EIO;
    }

    // turn off the cell modem after we're done searching for a signal/time
    Cellular.off();
    Particle.disconnect();
    

    return retval;
}
int MfgTest::wet_dry_sensor_test(void)
{
    int retval = 0;

    SF_OSAL_printf("Running the Wet/Dry Sensor\r\n");
    SF_OSAL_printf("Internally shorting the Wet/Dry Sensor\r\n");

    pSystemDesc->pWaterSensor->resetArray();
    // change window to small window (smaller moving average for quick detect)
    pSystemDesc->pWaterSensor->setWindowSize(WATER_DETECT_SURF_SESSION_INIT_WINDOW);
    // set the initial state to "not in water" (because hystersis)
    pSystemDesc->pWaterSensor->forceState(WATER_SENSOR_LOW_STATE);
    #if ICM20648_ENABLED
        // set in-water
        digitalWrite(WATER_MFG_TEST_EN, HIGH);
    #endif

    for (int i = 0; i < 100; i++)
    {
        pSystemDesc->pWaterSensor->takeReading();
    }
    if (!pSystemDesc->pWaterSensor->getCurrentStatus())
    {
        SF_OSAL_printf("Wet Sensor failed\r\n");
        retval = -EIO;
    }
    else
    {
        SF_OSAL_printf("Wet Sensor passed\r\n");
    }
    #if ICM20648_ENABLED
        // set out-of-water
        digitalWrite(WATER_MFG_TEST_EN, LOW);
    #endif

    //Take 100 readings, then query the status
    for (int i = 0; i < 100; i++)
    {
        pSystemDesc->pWaterSensor->takeReading();
    }
    if (pSystemDesc->pWaterSensor->getCurrentStatus())
    {
        SF_OSAL_printf("Dry Sensor failed\r\n");
        retval = -EIO;
    }
    else
    {
        SF_OSAL_printf("Dry Sensor passed\r\n");
    }

    #if ICM20648_ENABLED
        // set normal operation
        digitalWrite(WATER_MFG_TEST_EN, LOW);
    #endif

    return retval;
}
int MfgTest::status_led_test(void)
{
    LEDStatus rgbLED;
    typedef struct {
        uint32_t color;
        const char* colorName;
    } LED_ColorTest;
    const LED_ColorTest colorTests[] = {
        {RGB_COLOR_WHITE, "WHITE"},
        {RGB_COLOR_RED, "RED"},
        {RGB_COLOR_GREEN, "GREEN"},
        {RGB_COLOR_BLUE, "BLUE"},
        {0, NULL}
    };
    char prompt[80];

    SF_OSAL_printf("Running the User LED Test\r\n");

    rgbLED.setActive();
    for(const LED_ColorTest* pTest = colorTests; pTest->colorName; pTest++)
    {
        rgbLED.setPriority(LED_PRIORITY_CRITICAL);
        rgbLED.setPattern(LED_PATTERN_SOLID);
        rgbLED.setPeriod(3000);
        rgbLED.setColor(pTest->color);
        rgbLED.setActive();
        snprintf(prompt, 80, "Is the User LED %s?", pTest->colorName);
        if(!verify_yes_or_no(prompt))
        {
            SF_OSAL_printf("User LED failed!\n");
            rgbLED.setActive(false);
            return -EIO;
        }
    }
    rgbLED.setActive(false);
    SF_OSAL_printf("\n");

    return 0;
}
int MfgTest::battery_status_test(void)
{
    return led_test(pSystemDesc->pBatteryLED);
}
int MfgTest::battery_voltage_test(void)
{
    int retval = 0;
    float voltage = pSystemDesc->pBattery->getVCell();
    float battery_percentage = pSystemDesc->pBattery->getSoC();

    SF_OSAL_printf("Running the Battery LED Test\r\n");

    if ((voltage >= SF_BATTERY_SHUTDOWN_VOLTAGE) && (voltage <= SF_BATTERY_MAX_VOLTAGE))
    {
        SF_OSAL_printf("Battery Voltage passed\r\n");
        SF_OSAL_printf("Battery = %lf %%\r\n", battery_percentage);
        SF_OSAL_printf("Battery = %lf V\r\n", voltage);
    }
    else
    {
        SF_OSAL_printf("Battery Voltage failed\r\n", voltage);
        retval = -EIO;
    }

    return retval;
}
static bool verify_yes_or_no(const char * prompt_text)
{
    char selection;
    while (1)
    {
        SF_OSAL_printf("\r\n%s\r\n", prompt_text);
        SF_OSAL_printf("Press Y for yes or N for no: ");
        selection = getch();
        SF_OSAL_printf("\n");
        if (selection == 'Y' || selection == 'y')
        {
            return true;
        }
        if (selection == 'N' || selection == 'n')
        {
            return false;
        }
        SF_OSAL_printf("Invalid selection!\r\n");
    }
}

static int led_test(SFLed* pin)
{
    int retval = 0;
    bool result;

    SF_OSAL_printf("Running the Battery LED Test\r\n");

    // turn LED on
    pin->setState(SFLed::SFLED_STATE_ON);

    result = verify_yes_or_no("Is the Battery LED GREEN?");
    if (!result)
    {        SF_OSAL_printf("User LED failed!\r\n");
        return -EIO;
    }
    else
    {
        SF_OSAL_printf("User LED passed!\r\n");
    }

    // turn LED off
    pin->setState(SFLed::SFLED_STATE_OFF);

    return retval;
}