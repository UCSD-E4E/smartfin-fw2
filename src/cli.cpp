#include "cli.hpp"

#include "Particle.h"
#include <SpiffsParticleRK.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "conio.hpp"
#include "product.hpp"
#include "states.hpp"
#include "vers.hpp"
#include "fileCLI.hpp"
#include "system.hpp"
#include "sleepTask.hpp"

typedef const struct CLI_menu_
{
    char cmd;
    void (*fn)(void);
} CLI_menu_t;

typedef const struct CLI_debugMenu_
{
    const char *const fnName;
    int (*fn)(void);
} CLI_debugMenu_t;

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


const CLI_menu_t CLI_menu[12] =
    {
        {'#', &CLI_displayMenu},
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
        {'\0', NULL}};

static int CLI_displaySystemDesc(void);
static int CLI_testSleepLoadBoot(void);
static int CLI_setLEDs(void);
static int CLI_monitorSensors(void);
static int CLI_gpsTerminal(void);

const CLI_debugMenu_t CLI_debugMenu[] =
{
    {"Display System Desc", CLI_displaySystemDesc},
    {"Load Boot Behavior", CLI_testSleepLoadBoot},
    {"Set LED State", CLI_setLEDs},
    {"Monitor Sensors", CLI_monitorSensors},
    {"GPS Terminal", CLI_gpsTerminal},
    {NULL, NULL}
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
    while (millis() < lastKeyPressTime + CLI_NO_INPUT_TIMEOUT_MS && CLI_nextState == STATE_CLI)
    {
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
    CLI_ledStatus.setActive(false);
}

static void CLI_displayMenu(void)
{
    SF_OSAL_printf(
        "T for MFG Test, C for C for Calibrate Mode, B for Battery State,\n"
        "I for Init Surf Session, U for Data Upload, D for Deep Sleep,\n"
        "F for Format Flash, Z to check filesytem, L for List Files,\n"
        "R for Read/Delete/Copy Files, M for Make Files\n");
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
    const int nBytes = 496*3 + 3;
    int i, j;

    for (i = 0; i < nFiles; i++)
    {
        snprintf(fname, 31, fnameFmt, i);
        bin_file = pSystemDesc->pFileSystem->openFile(fname, SPIFFS_O_RDWR | SPIFFS_O_CREAT);
        for (j = 0; j < nBytes; j++)
        {
            bin_file.write(j & 0xFF);
        }
        bin_file.flush();
        bin_file.close();
    }
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
        cmdInput = atoi(inputBuffer);
        if(0 == cmdInput)
        {
            return;
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
        if(i == cmd)
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
    int32_t intPinState = 0;
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
        if(digitalRead(MPU_INT_PIN) == 0)
        {
            pSystemDesc->pIMU->get_accelerometer(accelData, accelData + 1, accelData + 2);
            pSystemDesc->pIMU->get_accel_raw_data((uint8_t*) accelRawData);

            pSystemDesc->pIMU->get_gyroscope(gyroData, gyroData + 1, gyroData + 2);
            pSystemDesc->pIMU->get_gyro_raw_data((uint8_t*) gyroRawData);
            
            pSystemDesc->pCompass->read(magData, magData + 1, magData + 2);
            pSystemDesc->pCompass->read((uint8_t*) magRawData);

            temp = pSystemDesc->pTempSensor->getTemp();

            waterDetect = pSystemDesc->pWaterSensor->getCurrentReading();

            location[0] = pSystemDesc->pGPS->location.lat();
            location[1] = pSystemDesc->pGPS->location.lng();
            if(!pSystemDesc->pGPS->location.isValid())
            {
                location[0] = NAN;
                location[1] = NAN;
            }

            // SF_OSAL_printf("Time between: %08.2f\r", elapsed / count);
            SF_OSAL_printf("%6d\t%8.4f\t%8.4f\t%8.4f\t%8.4f\t%8.4f\t%8.4f\t%8d\t%8d\t%8d\t%8.4f\t%8d\t%10.6f\t%10.6f\n", millis(), 
                accelData[0], accelData[1], accelData[2],
                gyroData[0], gyroData[1], gyroData[2],
                magData[0], magData[1], magData[2],
                temp, waterDetect, location[0], location[1]);
        }
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
    digitalWrite(GPS_PWR_EN_PIN, LOW);

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
    digitalWrite(GPS_PWR_EN_PIN, HIGH);
    return 1;
}