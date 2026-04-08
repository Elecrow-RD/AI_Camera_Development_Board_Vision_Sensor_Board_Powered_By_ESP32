#ifndef TOOL_LED_H
#define TOOL_LED_H

#include "esp_err.h"
#include <stddef.h>

/**
 * @file tool_led.h
 * @brief MimiClaw LED 工具封装层
 * * 该文件定义了 AI Agent 调用的标准接口。
 * 实现逻辑在 tool_led.c 中，通过调用底层的 aw9523 驱动来控制硬件。
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Initialize LED-related hardware.
 * * This function performs the following steps in sequence:
 * 1. Initializes the I2C bus (i2c_bus_init).
 * 2. Configures the AW9523 chip registers (aw9523_hw_init).
 * * @return 
 * - ESP_OK: Hardware is initialized and ready.
 * - ESP_FAIL: Hardware initialization failed.
 */
esp_err_t tool_led_init(void);

/**
 * @brief Agent Tool: Execute LED control via JSON input
 * @param input_json JSON string, e.g., {"action": "blink", "interval_ms": 200, "duration_ms": 3000}
 * @param output Result message buffer for the Agent
 * @param output_size Size of the output buffer
 * @return ESP_OK on success
 */
esp_err_t tool_led_control_execute(const char *input_json, char *output, size_t output_size);



#ifdef __cplusplus
}
#endif

#endif // TOOL_LED_H