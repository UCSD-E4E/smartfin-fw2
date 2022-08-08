#include "cli.hpp"

#include "Particle.h"
#include <SpiffsParticleRK.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <inttypes.h>
#include <vector>

#include "conio.hpp"
#include "product.hpp"
#include "states.hpp"
#include "vers.hpp"
#include "fileCLI.hpp"
#include "system.hpp"
#include "sleepTask.hpp"
#include "flog.hpp"
#include "mfgTest.hpp"
#include "utils.hpp"
#include "ensembleTypes.hpp"

typedef const struct CLI_menu_
{
    char cmd;
    void (*fn)(void);
} CLI_menu_t;

typedef const struct CLI_debugMenu_
{
    int cmd;
    const char *const fnName;
    int (*fn)(void);
} CLI_debugMenu_t;

const char* CAL_CONST_TYPES[] = {"gyro_intercept", "acc_coeff", "acc_intercept", "mag_coeff", "mag_intercept", "thermal_coeff", "thermal_intercept"};

static STATES_e CLI_nextState;
static LEDStatus CLI_ledStatus;
static int CLI_debugRun;

static void CLI_displayMenu(void);
static void CLI_doSleep(void);
static void CLI_doMfgTest(void);
static void CLI_doUpload(void);
static void CLI_doSessionInit(void);
static void CLI_doListDir(void);
static void CLI_doFormatFlash(void);
static void CLI_doCheckFS(void);
static void CLI_doMakeTestFiles(void);
static void CLI_doReadDeleteFiles(void);
static void CLI_doDebugMode(void);
static  CLI_menu_t const* CLI_findCommand(char const* const cmd, CLI_menu_t const* const menu);
static int CLI_executeDebugMenu(const int cmd, CLI_debugMenu_t* menu);
static void CLI_displayDebugMenu(CLI_debugMenu_t* menu);
static void CLI_doCalibrateMode(void);
static void CLI_set_no_upload_flag(void);
static void CLI_disable_no_upload_flag(void);
static void CLI_view_no_upload_flag(void);
static void CLI_load_cal_coeffs(void);
static void CLI_view_cal_coeffs(void);
static void CLI_exit(void);

const CLI_menu_t CLI_menu[19] =
    {
        {'#', &CLI_displayMenu},
        {'C', &CLI_doCalibrateMode},
        {'D', &CLI_doSleep},
        {'T', &CLI_doMfgTest},
        {'U', &CLI_doUpload},
        {'I', &CLI_doSessionInit},
        {'L', &CLI_doListDir},
        {'F', &CLI_doFormatFlash},
        {'Z', &CLI_doCheckFS},
        {'M', &CLI_doMakeTestFiles},
        {'R', &CLI_doReadDeleteFiles},
        {'*', &CLI_doDebugMode},
        {'H', &CLI_set_no_upload_flag},
        {'O', &CLI_disable_no_upload_flag},
        {'V', &CLI_view_no_upload_flag},
        {'X', &CLI_exit},
        {'~', &CLI_load_cal_coeffs},
        {'!', &CLI_view_cal_coeffs},
        {'\0', NULL}};

static int CLI_displaySystemDesc(void);
static int CLI_testSleepLoadBoot(void);
static int CLI_setLEDs(void);
static int CLI_monitorSensors(void);
static int CLI_gpsTerminal(void);
static int CLI_testWaterDetect(void);
static int CLI_connect(void);
static int CLI_disconnect(void);
static int CLI_displayVersion(void);
static int CLI_restart(void);
static int CLI_displayFLOG(void);
static int CLI_clearFLOG(void);
static int CLI_executeMfgPeripheralTest(void);
static int CLI_testSleep(void);

const CLI_debugMenu_t CLI_debugMenu[] =
{
    {1, "Display System Desc", CLI_displaySystemDesc},
    {2, "Load Boot Behavior", CLI_testSleepLoadBoot},
    {3, "Set LED State", CLI_setLEDs},
    {4, "Monitor Sensors", CLI_monitorSensors},
    {5, "GPS Terminal", CLI_gpsTerminal},
    {6, "Test Water Detect", CLI_testWaterDetect},
    {7, "Cloud connect", CLI_connect},
    {8, "Cloud disconnect", CLI_disconnect},
    {9, "Display Version", CLI_displayVersion},
    {10, "Restart", CLI_restart},
    {11, "Display Fault Log", CLI_displayFLOG},
    {12, "Clear Fault Log", CLI_clearFLOG},
    {13, "Execute Mfg Peripheral Test", CLI_executeMfgPeripheralTest},
    {14, "Test Sleep", CLI_testSleep},
    {0, NULL, NULL}
};

void CLI::init(void)
{
    VERS_printBanner();
    SF_OSAL_printf("Press # to list menu options\n");
    CLI_nextState = STATE_CLI;

    CLI_ledStatus.setColor(CLI_RGB_LED_COLOR);
    CLI_ledStatus.setPattern(CLI_RGB_LED_PATTERN);
    CLI_ledStatus.setPeriod(CLI_RGB_LED_PERIOD);
    CLI_ledStatus.setPriority(CLI_RGB_LED_PRIORITY);
    CLI_ledStatus.setActive();
    pSystemDesc->pChargerCheck->start();

    while(kbhit())
    {
        getch();
    }
}

STATES_e CLI::run(void)
{
    uint32_t lastKeyPressTime;
    char inputBuffer[80];
    CLI_menu_t *cmd;
    int i = 0;
    char userInput;

    CLI_nextState = STATE_CLI;

    lastKeyPressTime = millis();
    SF_OSAL_printf(">");
    while (1)
    {
        if(millis() >= lastKeyPressTime + CLI_NO_INPUT_TIMEOUT_MS) {
            CLI_nextState = STATE_CHARGE;
        }

        if(!pSystemDesc->flags->hasCharger) {
            return STATE_DEEP_SLEEP;
        }

        if(pSystemDesc->pWaterSensor->getCurrentStatus())
        {
            CLI_nextState = STATE_SESSION_INIT;
        }
        
        if (CLI_nextState != STATE_CLI)
        {
            break;
        }
        if (kbhit())
        {
            userInput = getch();
            lastKeyPressTime = millis();
            switch (userInput)
            {
            case '\b':
                i--;
                SF_OSAL_printf("\b \b");
                break;
            case '\r':
            case '\n':
                inputBuffer[i++] = 0;
                SF_OSAL_printf("\r\n");
                cmd = CLI_findCommand(inputBuffer, CLI_menu);
                if (!cmd)
                {
                    SF_OSAL_printf("Unknown command\n");
                }
                else
                {
                    cmd->fn();
                }
                SF_OSAL_printf(">");
                i = 0;
                break;
            default:
                inputBuffer[i++] = userInput;
                putch(userInput);
                break;
            }
        }
    }
    return CLI_nextState;
}

void CLI::exit(void)
{
    pSystemDesc->pChargerCheck->stop();
    CLI_ledStatus.setActive(false);
}

static void CLI_displayMenu(void)
{
    SF_OSAL_printf(
        "T for MFG Test, C for C for Calibrate Mode, B for Battery State,\n"
        "I for Init Surf Session, U for Data Upload, D for Deep Sleep,\n"
        "F for Format Flash, Z to check filesytem, L for List Files,\n"
        "R for Read/Delete/Copy Files, M for Make Files,\n"
        "H to set no_upload mode, O to disable no_upload mode, V to view no_upload flag,\n"
        "X to exit command line\n");
}

static CLI_menu_t const* CLI_findCommand( char const* const cmd, CLI_menu_t const* const menu)
{
    CLI_menu_t const* pCmd;

    for (pCmd = menu; pCmd->cmd; pCmd++)
    {
        if (pCmd->cmd == cmd[0])
        {
            return pCmd;
        }
    }
    return NULL;
}

static void CLI_doSleep(void)
{
    SF_OSAL_printf("Next state is sleep\n");
    CLI_nextState = STATE_DEEP_SLEEP;
}

static void CLI_doMfgTest(void)
{
    CLI_nextState = STATE_MFG_TEST;
}

static void CLI_doUpload(void)
{
    CLI_nextState = STATE_UPLOAD;
}

static void CLI_doSessionInit(void)
{
    CLI_nextState = STATE_SESSION_INIT;
}
static void CLI_doListDir(void)
{
    spiffs_DIR dir;
    int i;
    spiffs_dirent dirEnt;

    if (!pSystemDesc->pFileSystem->opendir(".", &dir))
    {
        SF_OSAL_printf("SPI Flash opendir fail with error code %d\n", pSystemDesc->pFileSystem->spiffs_errno());
        return;
    }

    for (i = 0; pSystemDesc->pFileSystem->readdir(&dir, &dirEnt); i++)
    {
        SF_OSAL_printf("%s\t%d\n", dirEnt.name, dirEnt.size);
    }

    pSystemDesc->pFileSystem->closedir(&dir);
}

static void CLI_doFormatFlash(void)
{
    SF_OSAL_printf("attempting format of SPI flash (60 seconds or so)...\n");
    Log.info("About to erase flash sectors");

    pSystemDesc->pFileSystem->unmount();
    pSystemDesc->pFileSystem->erase();

    if (SPIFFS_ERR_NOT_A_FS != pSystemDesc->pFileSystem->mount(NULL))
    {
        SF_OSAL_printf("*mount error\n");
        return;
    }
    SF_OSAL_printf("*mount success\n");

    if (SPIFFS_OK != pSystemDesc->pFileSystem->format())
    {
        SF_OSAL_printf("*format error\n");
        return;
    }
    SF_OSAL_printf("*format success\n");

    if (SPIFFS_OK != pSystemDesc->pFileSystem->mount(NULL))
    {
        SF_OSAL_printf("*mount error\n");
        return;
    }
    SF_OSAL_printf("*mount success\n");
}

static void CLI_doCheckFS(void)
{
    SF_OSAL_printf("checking spi flash filesystem (60 seconds or so)...\n");
    pSystemDesc->pFileSystem->check();
    SF_OSAL_printf("Done!\n");
}

static void CLI_doMakeTestFiles(void)
{
    SpiffsParticleFile bin_file;

    char fname[31];
    const char *const fnameFmt = "t%d.txt";
    const int nFiles = 3;
    const int nBytes = 496;
    uint8_t data[nBytes];
    int i, j;

    for(i = 0; i < nBytes; i++)
    {
        data[i] = i & 0xFF;
    }

    for (i = 0; i < nFiles; i++)
    {
        memset(fname, 0, 31);
        snprintf(fname, 31, fnameFmt, i);
        bin_file = pSystemDesc->pFileSystem->openFile(fname, SPIFFS_O_RDWR | SPIFFS_O_CREAT);
        memcpy(data, fname, 31);
        for (j = 0; j < nBytes; j++)
        {
            bin_file.write(data[j]);
        }
        bin_file.flush();
        bin_file.close();
    }

    sprintf(fname, ".test.txt");
    memcpy(data, fname, 31);
    bin_file = pSystemDesc->pFileSystem->openFile(fname, SPIFFS_O_RDWR | SPIFFS_O_CREAT);
    for (j = 0; j < nBytes; j++)
    {
        bin_file.write(data[j]);
    }
    bin_file.flush();
    bin_file.close();
    SF_OSAL_printf("Done making %d temp files!\n", nFiles);
}

static void CLI_doReadDeleteFiles(void)
{
    FileCLI app;
    app = FileCLI();
    app.execute();
}

static void CLI_doDebugMode(void)
{
    char inputBuffer[80];
    int cmdInput;

    CLI_debugRun = 1;
    while(CLI_debugRun)
    {
        CLI_displayDebugMenu(CLI_debugMenu);
        SF_OSAL_printf("*>");
        getline(inputBuffer, 80);
        if(strlen(inputBuffer) == 0)
        {
            continue;
        }
        cmdInput = atoi(inputBuffer);
        if(0 == cmdInput)
        {
            CLI_debugRun = 0;
        }
        CLI_executeDebugMenu(cmdInput, CLI_debugMenu);
    }
    return;
}

static int CLI_executeDebugMenu(const int cmd, CLI_debugMenu_t* menu)
{
    int i;

    for(i = 1; menu[i - 1].fn; i++)
    {
        if(menu[i - 1].cmd == cmd)
        {
            SF_OSAL_printf("Executing %s\n", menu[i-1].fnName);
            return menu[i-1].fn();
        }
    }
    SF_OSAL_printf("Entry not found\n");
    return 0;
}
static void CLI_displayDebugMenu(CLI_debugMenu_t* menu)
{
    int i;
    for(i = 1; menu[i-1].fn; i++)
    {
        SF_OSAL_printf("%3d %s\n", i, menu[i-1].fnName);
    }
    SF_OSAL_printf("%3d Exit\n", 0);
    return;
}

static int CLI_displaySystemDesc(void)
{
    SF_OSAL_printf("fileSystem: 0x%08x\n", pSystemDesc->pFileSystem);
    SF_OSAL_printf("Reset Reason: %x\n", System.resetReason());
    switch(System.resetReason())
    {
        case RESET_REASON_DFU_MODE:
            SF_OSAL_printf("    RESET_REASON_DFU_MODE\n");
            break;
        case RESET_REASON_POWER_MANAGEMENT:
            SF_OSAL_printf("    RESET_REASON_POWER_MANAGEMENT\n");
            break;
        case RESET_REASON_POWER_DOWN:
            SF_OSAL_printf("    RESET_REASON_POWER_DOWN\n");
            break;
        case RESET_REASON_POWER_BROWNOUT:
            SF_OSAL_printf("    RESET_REASON_POWER_BROWNOUT\n");
            break;
        case RESET_REASON_WATCHDOG:
            SF_OSAL_printf("    RESET_REASON_WATCHDOG\n");
            break;
        case RESET_REASON_UPDATE:
            SF_OSAL_printf("    RESET_REASON_UPDATE\n");
            break;
        case RESET_REASON_UPDATE_TIMEOUT:
            SF_OSAL_printf("    RESET_REASON_UPDATE_TIMEOUT\n");
            break;
        case RESET_REASON_FACTORY_RESET:
            SF_OSAL_printf("    RESET_REASON_FACTORY_RESET\n");
            break;
        case RESET_REASON_SAFE_MODE:
            SF_OSAL_printf("    RESET_REASON_SAFE_MODE\n");
            break;
        case RESET_REASON_PIN_RESET:
            SF_OSAL_printf("    RESET_REASON_PIN_RESET\n");
            break;
        case RESET_REASON_PANIC:
            SF_OSAL_printf("    RESET_REASON_PANIC\n");
            break;
        case RESET_REASON_USER:
            SF_OSAL_printf("    RESET_REASON_USER\n");
            break;
        case RESET_REASON_UNKNOWN:
            SF_OSAL_printf("    RESET_REASON_UNKNOWN\n");
            break;
        case RESET_REASON_NONE:
            SF_OSAL_printf("    RESET_REASON_NONE\n");
            break;
    }
    return 1;
}

static int CLI_testSleepLoadBoot(void)
{
    SleepTask::BOOT_BEHAVIOR_e behavior;
    int input;
    char cmdBuf[80];

    SF_OSAL_printf("Enter test behavior: ");
    getline(cmdBuf, 80);
    if(1 != sscanf(cmdBuf, "%d", &input))
    {
        return 0;
    }
    SleepTask::setBootBehavior((SleepTask::BOOT_BEHAVIOR_e) input);

    SleepTask::loadBootBehavior();
    behavior = SleepTask::getBootBehavior();
    SF_OSAL_printf("Behavior: %d\n", behavior);
    return 1;
}

static int CLI_setLEDs(void)
{
    char buffer[80];
    int led, state;

    SF_OSAL_printf("Set LEDs\n");
    SF_OSAL_printf("0: Water Detect LED\n");
    SF_OSAL_printf("1: Battery Charge LED\n");
    SF_OSAL_printf("Which led and state: ");
    getline(buffer, 80);
    if(2 != sscanf(buffer, "%d %d", &led, &state))
    {
        return 0;
    }
    switch(led)
    {
        case 0:
            pSystemDesc->pWaterLED->setState((SFLed::SFLED_State_e) state);
            return 1;
        case 1:
            pSystemDesc->pBatteryLED->setState((SFLed::SFLED_State_e) state);
            return 1;
        default:
        return 0;
    }
}

static int CLI_monitorSensors(void)
{
    uint16_t accelRawData[3];
    uint16_t gyroRawData[3];
    uint16_t magRawData[3];
    float accelData[3];
    float gyroData[3];
    int16_t magData[3];
    float temp;
    uint8_t waterDetect;
    double location[2];
  
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
    
    digitalWrite(GPS_PWR_EN_PIN, HIGH);
    delay(500);

    pSystemDesc->pGPS->gpsModuleInit();
    if(!pSystemDesc->pGPS->isEnabled())
    {
        SF_OSAL_printf("GPS Fail\n");
    }

    SF_OSAL_printf("%6s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%10s\t%10s\n", "time", "xAcc", "yAcc", "zAcc", "xAng", "yAng", "zAng", "xMag", "yMag", "zMag", "temp", "water", "lat", "lon");
    // SF_OSAL_printf("%6s\t%8s\n", "time", "temp");
    while(!kbhit())
    {
        while(GPS_kbhit())
        {
            pSystemDesc->pGPS->encode(GPS_getch());
        }
        pSystemDesc->pIMU->get_accelerometer(accelData, accelData + 1, accelData + 2);
        pSystemDesc->pIMU->get_accel_raw_data((uint8_t*) accelRawData);

        pSystemDesc->pIMU->get_gyroscope(gyroData, gyroData + 1, gyroData + 2);
        pSystemDesc->pIMU->get_gyro_raw_data((uint8_t*) gyroRawData);
        
        pSystemDesc->pCompass->read(magData, magData + 1, magData + 2);
        pSystemDesc->pCompass->read((uint8_t*) magRawData);

        temp = pSystemDesc->pTempSensor->getTemp();

        waterDetect = pSystemDesc->pWaterSensor->getLastReading();

        location[0] = pSystemDesc->pGPS->location.lat();
        location[1] = pSystemDesc->pGPS->location.lng();
        if(!pSystemDesc->pGPS->location.isValid())
        {
            location[0] = NAN;
            location[1] = NAN;
        }

        // SF_OSAL_printf("Time between: %08.2f\r", elapsed / count);
        SF_OSAL_printf("%6d\t%8.4f\t%8.4f\t%8.4f\t%8.4f\t%8.4f\t%8.4f\t%8d\t%8d\t%8d\t%8.4f\t%8d\t%10.6f\t%10.6f\r", millis(), 
            accelData[0], accelData[1], accelData[2],
            gyroData[0], gyroData[1], gyroData[2],
            magData[0], magData[1], magData[2],
            temp, waterDetect, location[0], location[1]);
    }
    SF_OSAL_printf("\n");
    while(kbhit())
    {
        getch();
    }
    pSystemDesc->pTempSensor->stop();
    pSystemDesc->pCompass->close();
    pSystemDesc->pIMU->close();
    pSystemDesc->pGPS->gpsModuleStop();
    digitalWrite(GPS_PWR_EN_PIN, LOW);

    return 1;
}

static int CLI_gpsTerminal(void)
{
    bool run = true;
    char user;
    digitalWrite(GPS_PWR_EN_PIN, HIGH);
    delay(500);
    pSystemDesc->pGPS->gpsModuleInit();

    while(run)
    {
        if(GPS_kbhit())
        {
            putch(GPS_getch());
        }
        if(kbhit())
        {
            user = getch();
            if(user == 27)
            {
                run = false;
            }
            else
            {
                GPS_putch(user);
            }
        }
    }
    digitalWrite(GPS_PWR_EN_PIN, LOW);
    return 1;
}

static int CLI_testWaterDetect(void)
{
    bool run = true;
    char user;
    while(run)
    {
        pSystemDesc->pWaterSensor->getLastReading();
        user = getch();
        if(user == '\x1b')
        {
            run = false;
        }
    }
    return 1;
}

static int CLI_connect(void)
{
    Particle.connect();
    waitFor(Particle.connected, 300000);
    return 1;
}
static int CLI_disconnect(void)
{
    Particle.disconnect();
    return 1;
}

static int CLI_displayVersion(void)
{
    VERS_printBanner();
    return 1;
}

static int CLI_restart(void)
{
    System.reset();
    return 1;
}

static int CLI_displayFLOG(void)
{
    FLOG_DisplayLog();
    return 1;
}

static int CLI_clearFLOG(void)
{
    FLOG_ClearLog();
    return 1;
}

static void CLI_doCalibrateMode(void)
{
    char userInput[32];
    uint8_t numCalAttempts;
    uint32_t cycleTime, dataCollectionPeriod;
    if (pSystemDesc->pTime->isValid() == false)
    {
        SF_OSAL_printf("Please wait to get a valid time from the Particle network.\nWaiting.");
        Particle.connect();

        // while we don't get a valid time and haven't hit the timeout
        while ((Time.isValid() == false))
        {
            delay(1000);
            SF_OSAL_printf(".");
        }

        // turn off the cell modem after we're done searching for a signal/time
        Cellular.off();
    }

    if(!pSystemDesc->pTime->isValid())
    {
        return;
    }

    SF_OSAL_printf("\nParticle Time Lock!\n");

    SF_OSAL_printf("Please enter the number of temp cycles: ");
    getline(userInput, 32);
    if(1 != sscanf(userInput, "%hhu", &numCalAttempts))
    {
        SF_OSAL_printf("Unknown input!\n");
        return;
    }
    SF_OSAL_printf("Temp cycles stored = %hu\n", numCalAttempts);
    pSystemDesc->pNvram->put(NVRAM::TMP116_CAL_ATTEMPTS_TOTAL, numCalAttempts);
    

    SF_OSAL_printf("Please enter the cycle time in seconds (total time between each temp measurement): ");
    getline(userInput, 32);
    if(1 != sscanf(userInput, "%lu", &cycleTime))
    {
        SF_OSAL_printf("Unknown input!\n");
        return;
    }
    SF_OSAL_printf("Cycle time = %lu seconds\n", cycleTime);
    pSystemDesc->pNvram->put(NVRAM::TMP116_CAL_CYCLE_PERIOD_SEC, cycleTime);

    SF_OSAL_printf("Please enter the measurement time in seconds (total time taking measurements each cycle): ");
    getline(userInput, 32);
    if(1 != sscanf(userInput, "%lu", &dataCollectionPeriod))
    {
        SF_OSAL_printf("Unknown input!\n");
        return;
    }
    SF_OSAL_printf("Cycle time = %lu seconds\n", dataCollectionPeriod);
    pSystemDesc->pNvram->put(NVRAM::TMP116_CAL_DATA_COLLECTION_PERIOD_SEC, dataCollectionPeriod);

    // reset the calibration routine to attempt #1
    pSystemDesc->pNvram->put(NVRAM::TMP116_CAL_CURRENT_ATTEMPT, (uint8_t) 1);

    // remove previous calibration file if necessary
    pSystemDesc->pFileSystem->remove("TMP116_CAL");

    // setup calibration boot behavior
    SleepTask::setBootBehavior(SleepTask::BOOT_BEHAVIOR_TMP_CAL_START);
    SF_OSAL_printf("\nSystem will boot into calibration mode upon next power cycle\n");

    System.sleep(SLEEP_MODE_SOFTPOWEROFF);
    // CLI_nextState = STATE_TMP_CAL;
}

static int CLI_executeMfgPeripheralTest(void)
{
    int testNum = 0;
    int i = 0;
    const char* testName[] = {
        "GPS Test",
        "IMU Test",
        "Temperature Test",
        "Cellular Test",
        "Wet/Dry Sensor Test",
        "Status LED Test",
        "Battery Status Test",
        "Battery Voltage Test",
        NULL
    };
    char userInput[SF_OSAL_LINE_WIDTH];
    int retval;

    for(i = 0; testName[i]; i++)
    {
        SF_OSAL_printf("%2d: %s\n", i, testName[i]);
    }
    SF_OSAL_printf("Test Number: ");
    getline(userInput, SF_OSAL_LINE_WIDTH);

    if(sscanf(userInput, "%d", &testNum) != 1)
    {
        SF_OSAL_printf("Unknown input\n");
        return 0;
    }

    if(testNum >= i)
    {
        SF_OSAL_printf("Unknown test\n");
        return 0;
    }

    
    retval = MfgTest::MFG_TEST_TABLE[testNum]();
    SF_OSAL_printf("%s returned %d\n", testName[testNum], retval);
    return !retval;
}

static int CLI_testSleep(void)
{
    char userInput[SF_OSAL_LINE_WIDTH];
    SystemSleepConfiguration sleepConfig;
    system_tick_t sleepTime;
    time_t start, stop;
    SF_OSAL_printf("Time to sleep: ");
    getline(userInput, SF_OSAL_LINE_WIDTH);
    sscanf(userInput, "%lu", &sleepTime);
    SF_OSAL_printf("Sleeping for %u s\n", sleepTime);
    start = Time.now();
    UTIL_sleepUntil(millis() + sleepTime * 1000);
    stop = Time.now();
    do
    {
        SF_OSAL_printf("> ");
        getline(userInput, SF_OSAL_LINE_WIDTH);
    } while (strcmp(userInput, "continue"));
    SF_OSAL_printf("Actual sleep time: %d s\n", stop - start);
    SF_OSAL_printf("stop time: %d s\n", stop);
    SF_OSAL_printf("start time: %d s\n", start);
    
    return 1;
}

static void CLI_set_no_upload_flag(void) {
    if (!pSystemDesc->pNvram->put(NVRAM::NO_UPLOAD_FLAG, true)) {
        SF_OSAL_printf("error enabling no upload flag\n");
    }
    SF_OSAL_printf("no upload flag enabled successfully\n");
}

static void CLI_disable_no_upload_flag(void) {
    if (!pSystemDesc->pNvram->put(NVRAM::NO_UPLOAD_FLAG, false)) {
        SF_OSAL_printf("error disabling no upload flag\n");
        return;
    }
    SF_OSAL_printf("no upload flag disabled successfully\n");
}

static void CLI_view_no_upload_flag(void) {
    bool no_upload_flag;
    pSystemDesc->pNvram->get(NVRAM::NO_UPLOAD_FLAG, no_upload_flag);
    SF_OSAL_printf("no_upload flag: %d\n",  no_upload_flag);

}

static void CLI_load_cal_coeffs(void) {
    uint32_t coeffs[TOTAL_CAL_CONSTS];
    char inputBuffer[200];
    int currConstant = 0;

    SF_OSAL_printf("press enter to start\n");
    for (int i = 0; i < TOTAL_CAL_CONSTS; i++) {
        getline(inputBuffer, 200);

        uint32_t coef_;
        if (1 != sscanf(inputBuffer, "%lu", &coef_)) {
            SF_OSAL_printf("%s\n", CAL_CONST_TYPES[currConstant]);
            currConstant++;
            i--;
            continue;
        }

        coeffs[i] = coef_;
    }

    if (!pSystemDesc->pNvram->put(NVRAM::CAL_COEFFS, coeffs)) {
        SF_OSAL_printf("error loading calibration coefficients\n");
        return;
    }
    SF_OSAL_printf("calibration coefficients loaded successfully\n");
}

static void CLI_view_cal_coeffs(void) {
    uint32_t coeffs[TOTAL_CAL_CONSTS];
    if (!pSystemDesc->pNvram->get(NVRAM::CAL_COEFFS, coeffs)) {return;}
        
    for(int i = 0; i < TOTAL_CAL_CONSTS; i++) {
        SF_OSAL_printf("%lu\n", coeffs[i]);
    }

}

static void CLI_exit(void) {
    CLI_nextState = STATE_CHARGE;
}