#ifndef __MFGTEST_H__
#define __MFGTEST_H__

#include "task.hpp"

#define MFG_MIN_VALID_TEMPERATURE   15
#define MFG_MAX_VALID_TEMPERATURE   30

class MfgTest : public Task {
    public:
    /**
     * @brief 
     * 
     */
    void init(void);
    STATES_e run(void);
    void exit(void);

    static int gps_test(void);
    static int imu_test(void);
    static int temperature_sensor_test(void);
    static int cellular_test(void);
    static int wet_dry_sensor_test(void);
    static int status_led_test(void);
    static int battery_status_test(void);
    static int battery_voltage_test(void);
    static int(* MFG_TEST_TABLE[])(void);
    
};

#endif
