#include "AK09916.h"
#include "product.hpp"

#include "flog.hpp"

//
// Register addresses
//

#define AK09916_WIA2    (0x01)  // Device ID
#define AK09916_ST1     (0x10)  // Status 1
#define AK09916_HXL     (0x11)  // X-axis LSB
#define AK09916_HXH     (0x12)  // X-axis MSB
#define AK09916_HYL     (0x13)  // Y-axis LSB
#define AK09916_HYH     (0x14)  // Y-axis MSB
#define AK09916_HZL     (0x15)  // Z-axis LSB
#define AK09916_HZH     (0x16)  // Z-axis MSB
#define AK09916_ST2     (0x18)  // Status 2
#define AK09916_CNTL2   (0x31)  // Control 2
#define AK09916_CNTL3   (0x32)  // Control 3
#define AK09916_TS1     (0x33)  // Test 1 (DO NOT ACCESS)
#define AK09916_TS2     (0x34)  // Test 2 (DO NOT ACCESS)

//
// Register definitions
//

// Device ID
#define AK09916_DEV_ID              (0x09)

// Status 1
#define AK09916_ST1_DRDY            (0x01)  // Data Ready
#define AK09916_ST1_DOR             (0x02)  // Data Overrun

// Status 2
#define AK09916_ST2_HOFL            (0x08)  // Magnetic sensor overflow
#define AK09916_ST2_RSV28           (0x10)  // Reserved 28
#define AK09916_ST2_RSV29           (0x20)  // Reserved 29
#define AK09916_ST2_RSV30           (0x40)  // Reserved 30

// Control 2
#define AK09916_CNTL2_MODE          (0x1F)  // Control mode

#define AK09916_CNTL2_PWRDWN_MODE   (0x00)  // Power-down mode
#define AK09916_CNTL2_SINGLE_MODE   (0x01)  // Single measurement mode
#define AK09916_CNTL2_CONT1_MODE    (0x02)  // Continuous measurement mode 1
#define AK09916_CNTL2_CONT2_MODE    (0x04)  // Continuous measurement mode 2
#define AK09916_CNTL2_CONT3_MODE    (0x06)  // Continuous measurement mode 3
#define AK09916_CNTL2_CONT4_MODE    (0x08)  // Continuous measurement mode 4
#define AK09916_CNTL2_SELFTEST_MODE (0x10)  // Self-test mode

// Control 3
#define AK09916_CNTL3_SRST          (0x01)  // Soft reset

#define MEASUREMENT_TIMEOUT_MS      (300)

/**
 * @brief Sensor data size
 * 
 * [0x11:0x17]
 */
#define AK09916_SENSOR_DATA_SZ (6)



AK09916::AK09916(uint8_t address)
{
    m_address = address;
}

bool AK09916::open(void)
{
    // Check device ID
    uint8_t id;
    read_register(AK09916_WIA2, sizeof(id), &id);
    if (id != AK09916_DEV_ID)
    {
        FLOG_AddError(FLOG_MAG_ID_MISMATCH, 0);
        return false;
    }

    // Soft reset
    write_register(AK09916_CNTL3, AK09916_CNTL3_SRST);
    delay(50);

    // Set power down mode
    write_register(AK09916_CNTL2, AK09916_CNTL2_PWRDWN_MODE);
    delay(50);

    // Set to self test mode
    write_register(AK09916_CNTL2, AK09916_CNTL2_SELFTEST_MODE);

    // Wait no more than MEASUREMENT_TIMEOUT_MS for data ready
    system_tick_t start_ms = millis();
    while  (millis() - start_ms <= MEASUREMENT_TIMEOUT_MS)
    {
        // Check if data ready
        uint8_t reg;
        read_register(AK09916_ST1, sizeof(reg), &reg);
        if ((reg & AK09916_ST1_DRDY) == AK09916_ST1_DRDY)
        {
            break;
        }
    }
    if (millis() - start_ms > MEASUREMENT_TIMEOUT_MS)
    {
        FLOG_AddError(FLOG_MAG_MEAS_TO, 0);
        return false;
    }

    // Read test data
    uint8_t data[AK09916_SENSOR_DATA_SZ];
    read_register(AK09916_HXL, AK09916_SENSOR_DATA_SZ, data);
    int16_t mx = ((int16_t)data[1] << 8) | data[0];
    int16_t my = ((int16_t)data[3] << 8) | data[2];
    int16_t mz = ((int16_t)data[5] << 8) | data[4];

    // Check for valid test data ranges
    if ((mx < -200 || mx > 200) ||
        (my < -200 || my > 200) ||
        (mz < -1000 || mz > -200))
    {
        FLOG_AddError(FLOG_MAG_TEST_FAIL, 0);
        return false;
    }

    // Set to power down mode
    write_register(AK09916_CNTL2, AK09916_CNTL2_PWRDWN_MODE);
    
    return true;
}

void AK09916::close(void)
{
}

bool AK09916::read(int16_t* x, int16_t* y, int16_t* z)
{
    uint8_t data[AK09916_SENSOR_DATA_SZ];

    if (read(data) == false)
    {
        *x = -1;
        *y = -1;
        *z = -1;
        return false;
    }

    // Convert the MSB and LSB into a signed 16-bit value
    *x = ((int16_t)data[1] << 8) | data[0];
    *y = ((int16_t)data[3] << 8) | data[2];
    *z = ((int16_t)data[5] << 8) | data[4];

    return true;
}

bool AK09916::read(uint8_t* data)
{
    uint8_t reg;
    uint8_t reg1;

    // Set to single measurement mode
    write_register(AK09916_CNTL2, AK09916_CNTL2_SINGLE_MODE);
    read_register(AK09916_CNTL2, sizeof(uint8_t), &reg);
    if(AK09916_CNTL2_SINGLE_MODE != reg)
    {
        FLOG_AddError(FLOG_MAG_MODE_FAIL, reg);
        return false;
    }

    // Wait no more than MEASUREMENT_TIMEOUT_MS for data ready
    system_tick_t start_ms = millis();
    while  (millis() - start_ms <= MEASUREMENT_TIMEOUT_MS)
    {
        // Check for sensor overflow
        read_register(AK09916_ST2, sizeof(uint8_t), &reg);
        if (reg & AK09916_ST2_HOFL)
        {
            memset(data, -1, AK09916_SENSOR_DATA_SZ);
            FLOG_AddError(FLOG_MAG_MEAS_OVRFL, 0);
            return false;
        }

        // Check if data ready
        read_register(AK09916_ST1, sizeof(uint8_t), &reg);
        if ((reg & AK09916_ST1_DRDY) == AK09916_ST1_DRDY)
        {
            break;
        }
    }
    if (millis() - start_ms > MEASUREMENT_TIMEOUT_MS)
    {
        memset(data, -1, AK09916_SENSOR_DATA_SZ);
        read_register(AK09916_ST2, sizeof(uint8_t), &reg);
        read_register(AK09916_ST1, sizeof(uint8_t), &reg1);
        FLOG_AddError(FLOG_MAG_MEAS_TO, reg | (reg1 << 8));
        return false;
    }

    // Read data
    read_register(AK09916_HXL, AK09916_SENSOR_DATA_SZ, data);

    // Read STAT2 after reading data
    read_register(AK09916_ST2, sizeof(uint8_t), &reg);

    return true;
}

/***************************************************************************//**
 * @brief
 *    Reads register from the AK09916 device
 *
 * @param[in] addr
 *    The register address to read from in the sensor
 *
 * @param[in] numBytes
 *    The number of bytes to read
 *
 * @param[out] data
 *    The data read from the register
 *
 * @return
 *    None
 ******************************************************************************/
void AK09916::read_register(uint8_t addr, int numBytes, uint8_t* data)
{
    if(m_I2C.write(m_address, (const char*)&addr, sizeof(addr), true))
    {
        FLOG_AddError(FLOG_MAG_I2C_FAIL, 0);
        return;
    }
    if(m_I2C.read(m_address, (char*)data, numBytes, false))
    {
        FLOG_AddError(FLOG_MAG_I2C_FAIL, 1);
        return;
    }
}

/***************************************************************************//**
 * @brief
 *    Writes a register in the AK09916 device
 *
 * @param[in] addr
 *    The register address to write
 *
 * @param[in] data
 *    The data to write to the register
 *
 * @return
 *    None
 ******************************************************************************/
void AK09916::write_register(uint8_t addr, uint8_t data)
{
    uint8_t bytes[] = { addr, data };
    if(m_I2C.write(m_address, (const char*)bytes, sizeof(bytes), false))
    {
        FLOG_AddError(FLOG_MAG_I2C_FAIL, 2);
        return;
    }
}
