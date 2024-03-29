#ifndef __PRODUCT_HPP__
#define __PRODUCT_HPP__

/******************************************************************************
 * Pin Definitions
 *****************************************************************************/
/**
 * USB Power Detection Pin
 */
#define SF_USB_PWR_DETECT_PIN   RX
/**
 * Water Detect Enable Pin
 */
#define WATER_DETECT_EN_PIN   B4
/**
 * Water Detect Pin
 */
#define WATER_DETECT_PIN      B5
/**
 * @brief Manufacturing Water Detect Pin
 * 
 */
#define WATER_MFG_TEST_EN     A3
/**
 * Battery Charging Status Pin
 */
#define STAT_PIN              A5
/**
 * Pin for the Battery Status LED
 */
#define STAT_LED_PIN          A4
/**
 * Pin for the Water Detect State LED
 */
#define LED_PIN               C2
/**
 * @brief Wakeup Pin
 * 
 */
#define WKP_PIN               WKP
/**
 * @brief GPS Power Enable pin
 * 
 */
#define GPS_PWR_EN_PIN        B2

/**
 * @brief Accelerometer/Gyroscope Interrupt Pin
 * 
 */
#define MPU_INT_PIN             B3

/*******************************************************************************
 * Peripheral Configurations
 ******************************************************************************/
#define SERIAL_DEBUG_BAUD_RATE 115200
/**
 * SPI Flash Size
 */
#define SF_FLASH_SIZE_MB    4

/**
 * Charging voltage (mV)
 */
#define SF_CHARGE_VOLTAGE   4112

/**
 * @brief Below what battery voltage should the system shutdown
 * 
 */
#define SF_BATTERY_SHUTDOWN_VOLTAGE 3.0

/**
 * How long (in us) to turn on water detection circuit when looking for water
 */
#define WATER_DETECT_EN_TIME_US     1000

/**
 * @brief Particle IO device
 * 
 */
#define PARTICLE_IO 1
/**
 * @brief hardware revision
 * 
 */
#define HARDWARE_REV 2
/**
 * @brief enable ICM20648 motion sensor support
 * 
 */
#define ICM20648_ENABLED (HARDWARE_REV==2)

/**
 * @brief ICM20648 Address
 * 
 */
#define SF_ICM20648_ADDR    (0x68 << 1)

/**
 * @brief AK09916 Address
 * 
 */
#define SF_AK09916_ADDR (0x0C << 1)

/**
 * window sizes are how many water detect samples are looked at in a moving 
 * average to determine if we are in or out of the water.  Generally a sample
 * happens 1/second
 */
#define WATER_DETECT_SURF_SESSION_INIT_WINDOW   40
/**
 * Max size of the window/how large the FIFO array is
 */
#define WATER_DETECT_ARRAY_SIZE 200
/**
 * @brief How long to stay in surf session init while waiting to get into water
 * before going to sleep
 * 
 */
#define SURF_SESSION_GET_INTO_WATER_TIMEOUT_MS 300000

/*******************************************************************************
 * System Configuration
 ******************************************************************************/

/**
 * how long to stay in CLI mode if there are no key presses at all
 */
#define CLI_NO_INPUT_TIMEOUT_MS 600000

/**
 * The default state that the Smartfin comes up in
 */
#define SF_DEFAULT_STATE   STATE_CHARGE

/**
 * The CLI RGB LED Color
 */
#define SF_CLI_RGB_LED_COLOR        RGB_COLOR_RED
#define SF_CLI_RGB_LED_PATTERN      LED_PATTERN_SOLID
#define SF_CLI_RGB_LED_PERIOD       3000
#define SF_CLI_RGB_LED_PRIORITY     LED_PRIORITY_IMPORTANT

/**
 * The Data Upload RGB LED Color
 */
#define SF_DUP_RGB_LED_COLOR        RGB_COLOR_BLUE
#define SF_DUP_RGB_LED_PERIOD       500

#define SF_TCAL_RGB_LED_COLOR       RGB_COLOR_ORANGE
#define SF_TCAL_RGB_LED_PATTERN     LED_PATTERN_FADE
#define SF_TCAL_RGB_LED_PERIOD      3000
#define SF_TCAL_RGB_LED_PRIORITY    LED_PRIORITY_IMPORTANT

/**
 * Minimum battery voltage to start an upload
 */ 
#define SF_BATTERY_UPLOAD_VOLTAGE 3.6

/**
 * how long to stay in data upload without a cell signal/connection/succesful upload
 */
#define SF_CELL_SIGNAL_TIMEOUT_MS 300000



/**
 * @brief Seconds to sleep between upload attempts
 * 
 */
#define SF_UPLOAD_REATTEMPT_DELAY_SEC 600

/**
 * @brief how many ms is a GPS data point valid for a given data log
 * 
 */
#define GPS_AGE_VALID_MS 5000


/**
 * @brief How long to wait for a cell connection in during manufacturing test
 * 
 */
#define MANUFACTURING_CELL_TIMEOUT_MS 180000

/**
 * @brief A voltage that's slightly higher than the max battery voltage
 * 
 */
#define SF_BATTERY_MAX_VOLTAGE 4.3

/**
 * @brief Lost Bird Smartfin Z7 Product ID
 * 
 */
#define PRODUCT_ID_SMARTFIN_Z7  8977
/**
 * @brief UCSD Smartfin Product ID
 * 
 */
#define PRODUCT_ID_UCSD_SMARTFIN    17293

/**
 * @brief Set to use a hexadecimal product version, otherwise use a decimal
 * product version
 * 
 */
#define PRODUCT_VERSION_USE_HEX 0

/**
 * @brief Enable initialization delay
 * 
 */
// #define SF_ENABLE_DEBUG_DELAY   15

/**
 * @brief Base85 encoding flag
 * 
 */
#define SF_UPLOAD_BASE85 1
/**
 * @brief Base64 encoding flag
 * 
 */
#define SF_UPLOAD_BASE64 2
/**
 * @brief Base64url encoding flag
 * 
 */
#define SF_UPLOAD_BASE64URL 3

#define SF_UPLOAD_ENCODING SF_UPLOAD_BASE64URL

#endif