#pragma once

#include "Particle.h"
#include "product.hpp"
#include "i2c.h"
#include <stdint.h>

class AK09916
{
public:
    /** Create an AK09916 object with the specified I2C address
     *
     * @param address I2C address.
     */
    AK09916(uint8_t address);

    /** Open the instance
     *
     * @returns
     *   'true' if the open succeeded,
     *   'false' if the open failed.
     */
    bool open(void);

    /** Close the instance
     */
    void close(void);

    /** Read a measurement
     *
     * @param[out] x axis
     * @param[out] y axis
     * @param[out] z axis
     *
     * @returns
     *   'true' if the read succeeded,
     *   'false' if the read failed.
     */
    bool read(int16_t* x, int16_t* y, int16_t* z);

    /** Read a measurement
     *
     * @param[out] data must be of size SENSOR_DATA_SZ
     *
     * @returns
     *   'true' if the read succeeded,
     *   'false' if the read failed.
     */
    bool read(uint8_t* data);

private:
    // Private methods
    void read_register(uint8_t addr, int numBytes, uint8_t *data);
    void write_register(uint8_t addr, uint8_t data);

    // Private member variables
    I2C m_I2C;
    uint8_t m_address;
};
