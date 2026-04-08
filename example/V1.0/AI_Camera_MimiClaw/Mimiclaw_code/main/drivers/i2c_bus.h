#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <stdio.h>
#include <math.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c.h"

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  400000
#define I2C_SDA_IO          16  
#define I2C_SCL_IO          15

#define ACK_CHECK_EN        0x1 

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_write(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
esp_err_t i2c_bus_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);
esp_err_t i2c_bus_write_byte(uint8_t addr, uint8_t reg, uint8_t data);

#endif