#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "Particle.h"
#include "SpiffsParticleRK.h"
#include "nvram.hpp"
#include "waterSensor.hpp"
#include "led.hpp"
#include "recorder.hpp"
#include "TinyGPSMod.h"

#define SYS_CHARGER_MIN_CHARGING_MS 5000
#define SYS_CHARGER_MIN_CHARGED_MS 30000
#define SYS_CHARGER_REFRESH_MS  500
#define SYS_WATER_REFRESH_MS    1000
#define SYS_BATTERY_MONITOR_MS  1000

typedef volatile struct SystemFlags_
{
    bool batteryLow;
    bool hasCharger;
}SystemFlags_t;

typedef struct SystemDesc_
{
    SpiffsParticle* pFileSystem;
    PMIC* pmic;
    FuelGauge* battery;
    NVRAM* nvram;
    const char* deviceID;
    WaterSensor* pWaterSensor;
    SFLed* pBatteryLED;
    SFLed* pWaterLED;
    Timer* pChargerCheck;
    Timer* pWaterCheck;
    LEDSystemTheme* systemTheme;
    Recorder* pRecorder;
    TinyGPSPlus* pGPS;
    const SystemFlags_t* flags;
}SystemDesc_t;

extern SystemDesc_t* pSystemDesc;

int SYS_initSys(void);
int SYS_deinitSys(void);

#endif