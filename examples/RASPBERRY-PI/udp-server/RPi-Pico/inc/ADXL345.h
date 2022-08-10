#ifndef ADXL345_H
#define ADXL345_H
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

class Adxl345
{
    public:
        Adxl345(i2c_inst_t *i2c)
        : m_i2c{i2c}
        {}
        ~Adxl345() = default;

    public:
        bool init();
        bool read(int16_t *accelerometer_data, size_t size);

    private:
        i2c_inst_t * m_i2c;
};

#endif
