#include "aw9523.h"
#include "i2c_bus.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


uint8_t current_p1_out = 0x00;

esp_err_t aw9523_init(void) {
    // 1. 硬件复位逻辑 (保持不变)
    gpio_set_direction(AW9523_RSTN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(AW9523_RSTN_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(AW9523_RSTN_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2. 尝试读取 ID 寄存器
    uint8_t chip_id = 0;
    // 使用你之前定义的 i2c_bus_read 函数
    esp_err_t err = i2c_bus_read(AW9523_ADDR, 0x10, &chip_id, 1);
    
    if (err != ESP_OK) {
        ESP_LOGE("AW9523", "I2C Read Failed! Is the device connected?");
        return err;
    }

    if (chip_id != 0x23) {
        ESP_LOGE("AW9523", "ID Mismatch! Expected 0x23, got 0x%02X", chip_id);
        return ESP_FAIL;
    }

    ESP_LOGI("AW9523", "Hardware detected! ID: 0x%02X", chip_id);


    err |= i2c_bus_write_byte(AW9523_ADDR, All_Config_addr, 0x10);
    err |= i2c_bus_write_byte(AW9523_ADDR, REG_CONFIG_PORT1, 0xFF);
    err |= i2c_bus_write_byte(AW9523_ADDR, REG_CONFIG_PORT0, 0x01);
    err |= i2c_bus_write_byte(AW9523_ADDR, REG_OUT_PORT1, 0x09);

    if (err != ESP_OK) {
        ESP_LOGE("AW9523", "Configuration failed!");
        return ESP_FAIL;
    }

    i2c_bus_write_byte(AW9523_ADDR, REG_OUT_PORT1, 0x01);

    return ESP_OK;
}


esp_err_t aw9523_set_p1_gpio(bool level) {

    if (level) {
        current_p1_out |= (1 << 1);  
    } else {

        current_p1_out &= ~(1 << 1); 
    }

    //current_p1_out |= 0x09; 
    current_p1_out |= 0x01; 
    ESP_LOGI("AW9523", "Setting P1_1 to %s (OUT_PORT1=0x%02X)", level ? "ON" : "OFF", current_p1_out);
    return i2c_bus_write_byte(AW9523_ADDR, REG_OUT_PORT1, current_p1_out);
}

esp_err_t lcd_backlight_ctrl(bool level) {

    if (level) {
        current_p1_out &= ~(1 << 3); 
    } else {

        
        current_p1_out |= (1 << 3); 
    }

    current_p1_out |= 0x01; 
    ESP_LOGI("AW9523", "Setting P1_3 to %s (OUT_PORT1=0x%02X)", level ? "ON" : "OFF", current_p1_out);
    return i2c_bus_write_byte(AW9523_ADDR, REG_OUT_PORT1, current_p1_out);
}
