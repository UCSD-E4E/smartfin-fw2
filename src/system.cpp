#include "system.hpp"

#include <cstring>
#include "Particle.h"
#include "product.hpp"

static SpiFlashMacronix DP_spiFlash(SPI1, D5);
SpiffsParticle DP_fs(DP_spiFlash);
static PMIC pmic;
FuelGauge battery;
static WaterSensor waterSensor(WATER_DETECT_EN_PIN, WATER_DETECT_PIN, 
    WATER_DETECT_SURF_SESSION_INIT_WINDOW, WATER_DETECT_ARRAY_SIZE);

static SFLed batteryLED(STAT_LED_PIN, SFLed::SFLED_STATE_OFF);
static SFLed waterLED(LED_PIN,  SFLed::SFLED_STATE_OFF);
Recorder dataRecorder;

TinyGPSPlus SF_gps;

static void SYS_chargerTask(void);
static void SYS_waterTask(void);
static void SYS_batteryTask(void);
static Timer chargerTimer(SYS_CHARGER_REFRESH_MS, SYS_chargerTask, false);
static Timer waterTimer(SYS_WATER_REFRESH_MS, SYS_waterTask, false);
static Timer batteryMonitorTimer(SYS_BATTERY_MONITOR_MS, SYS_batteryTask, false);
static LEDSystemTheme ledTheme;

char SYS_deviceID[32];

SystemDesc_t systemDesc, *pSystemDesc = &systemDesc;
SystemFlags_t systemFlags;

static int SYS_initFS(void);
static int SYS_initPMIC(void);
static int SYS_initNVRAM(void);
static int SYS_initWaterSensor(void);
static int SYS_initLEDs(void);
static int SYS_initTasks(void);
static int SYS_initGPIO(void);

int SYS_initSys(void)
{
    memset(pSystemDesc, 0, sizeof(SystemDesc_t));
    systemDesc.deviceID = SYS_deviceID;
    systemDesc.flags = &systemFlags;

    memset(SYS_deviceID, 0, 32);
    strncpy(SYS_deviceID, System.deviceID(), 31);

    SYS_initFS();
    SYS_initPMIC();
    SYS_initNVRAM();
    SYS_initWaterSensor();
    SYS_initLEDs();
    SYS_initTasks();
    SYS_initGPIO();

    systemDesc.pGPS = &SF_gps;
    return 1;
}

static int SYS_initFS(void)
{
    DP_spiFlash.begin();
    DP_fs.withPhysicalAddr(SF_FLASH_SIZE_MB * 1024 * 1024);
    DP_fs.mount();
    systemDesc.pFileSystem = &DP_fs;

    dataRecorder.init();
    systemDesc.pRecorder = &dataRecorder;
    return 1;
}

int SYS_deinitSys(void)
{
    Cellular.off();
    DP_fs.unmount();
    DP_spiFlash.deepPowerDown();
    return 1;
}

static int SYS_initPMIC(void)
{
    pmic.setChargeVoltage(SF_CHARGE_VOLTAGE);
    pmic.setChargeCurrent(0, 0, 0, 0, 0, 0);
    systemDesc.pmic = &pmic;
    systemDesc.battery = &battery;
    return 1;
}

static int SYS_initNVRAM(void)
{
    NVRAM& nvram = NVRAM::getInstance();

    systemDesc.nvram = &nvram;

    return 1;
}
static int SYS_initWaterSensor(void)
{
    systemDesc.pWaterSensor = &waterSensor;
    return 1;
}

static int SYS_initLEDs(void)
{
    waterLED.init();
    batteryLED.init();

    ledTheme.setSignal(LED_SIGNAL_NETWORK_OFF, 0x000000, LED_PATTERN_SOLID);
    ledTheme.setSignal(LED_SIGNAL_NETWORK_ON, SF_DUP_RGB_LED_COLOR, LED_PATTERN_SOLID);
    ledTheme.setSignal(LED_SIGNAL_NETWORK_CONNECTING, SF_DUP_RGB_LED_COLOR, LED_PATTERN_SOLID);
    ledTheme.setSignal(LED_SIGNAL_NETWORK_DHCP, SF_DUP_RGB_LED_COLOR, LED_PATTERN_SOLID);
    ledTheme.setSignal(LED_SIGNAL_NETWORK_CONNECTED, SF_DUP_RGB_LED_COLOR, LED_PATTERN_SOLID);
    ledTheme.setSignal(LED_SIGNAL_CLOUD_CONNECTING, SF_DUP_RGB_LED_COLOR, LED_PATTERN_SOLID);
    ledTheme.setSignal(LED_SIGNAL_CLOUD_CONNECTED, SF_DUP_RGB_LED_COLOR, LED_PATTERN_BLINK, SF_DUP_RGB_LED_PERIOD);
    ledTheme.setSignal(LED_SIGNAL_CLOUD_HANDSHAKE, SF_DUP_RGB_LED_COLOR, LED_PATTERN_BLINK, SF_DUP_RGB_LED_PERIOD);
    
    systemDesc.pWaterLED = &waterLED;
    systemDesc.pBatteryLED = &batteryLED;
    systemDesc.systemTheme = &ledTheme;
    return 1;
}

static int SYS_initTasks(void)
{
    pinMode(STAT_PIN, INPUT);
    pinMode(SF_USB_PWR_DETECT_PIN, INPUT);
    systemFlags.hasCharger = true;
    systemFlags.batteryLow = false;

    systemDesc.pChargerCheck = &chargerTimer;
    systemDesc.pWaterCheck = &waterTimer;
    batteryMonitorTimer.start();
    return 1;
}

static void SYS_chargerTask(void)
{
    bool isCharging = ~digitalRead(STAT_PIN);
    systemFlags.hasCharger = digitalRead(SF_USB_PWR_DETECT_PIN);
    static int chargedTimestamp;
    static int chargingTimestamp;

    if(systemFlags.hasCharger)
    {
        // Check charging with hysteresis
        if(isCharging)
        {
            chargingTimestamp++;
            if(chargingTimestamp >= SYS_CHARGER_MIN_CHARGING_MS / SYS_CHARGER_REFRESH_MS)
            {
                chargingTimestamp = SYS_CHARGER_MIN_CHARGING_MS / SYS_CHARGER_REFRESH_MS;
            }
            chargedTimestamp = 0;
        }
        else
        {
            chargingTimestamp = 0;
            chargedTimestamp++;
            if(chargedTimestamp >= SYS_CHARGER_MIN_CHARGED_MS / SYS_CHARGER_REFRESH_MS)
            {
                chargedTimestamp = SYS_CHARGER_MIN_CHARGED_MS / SYS_CHARGER_REFRESH_MS;
            }
        }
        if(chargingTimestamp >= SYS_CHARGER_MIN_CHARGING_MS / SYS_CHARGER_REFRESH_MS)
        {
            systemDesc.pBatteryLED->setState(SFLed::SFLED_STATE_BLINK);
        }
        if(chargedTimestamp >= SYS_CHARGER_MIN_CHARGED_MS / SYS_CHARGER_REFRESH_MS)
        {
            systemDesc.pBatteryLED->setState(SFLed::SFLED_STATE_ON);
        }
    }
    else
    {
        chargedTimestamp = 0;
        chargedTimestamp = 0;
        systemDesc.pBatteryLED->setState(SFLed::SFLED_STATE_OFF);
        systemDesc.pChargerCheck->stopFromISR();
    }
}
static void SYS_waterTask(void)
{
    systemDesc.pWaterSensor->takeReading();
    if(systemDesc.pWaterSensor->getCurrentReading())
    {
        systemDesc.pWaterLED->setState(SFLed::SFLED_STATE_ON);
    }
    else
    {
        systemDesc.pWaterLED->setState(SFLed::SFLED_STATE_OFF);
    }
}

static int SYS_initGPIO(void)
{
    pinMode(WKP_PIN, INPUT_PULLDOWN);
    return 1;
}

static void SYS_batteryTask(void)
{
    if(systemDesc.battery->getVCell() < SF_BATTERY_SHUTDOWN_VOLTAGE)
    {
        systemFlags.batteryLow = true;
    }
}