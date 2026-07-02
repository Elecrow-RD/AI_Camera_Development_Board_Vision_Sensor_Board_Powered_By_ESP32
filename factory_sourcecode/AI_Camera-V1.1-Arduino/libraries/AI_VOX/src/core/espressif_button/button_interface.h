/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

// -DBUTTON_VER_MAJOR=4 -DBUTTON_VER_MINOR=1 -DBUTTON_VER_PATCH=1
#ifndef BUTTON_VER_MAJOR
#define BUTTON_VER_MAJOR (4)
#endif

#ifndef BUTTON_VER_MINOR
#define BUTTON_VER_MINOR (1)
#endif

#ifndef BUTTON_VER_PATCH
#define BUTTON_VER_PATCH (1)
#endif

#define CONFIG_BUTTON_PERIOD_TIME_MS 5
#define CONFIG_BUTTON_DEBOUNCE_TICKS 2
#define CONFIG_BUTTON_SHORT_PRESS_TIME_MS 180
#define CONFIG_BUTTON_LONG_PRESS_TIME_MS 1500
#define CONFIG_BUTTON_LONG_PRESS_HOLD_SERIAL_TIME_MS 20
#define CONFIG_ADC_BUTTON_MAX_CHANNEL 3
#define CONFIG_ADC_BUTTON_MAX_BUTTON_PER_CHANNEL 8
#define CONFIG_ADC_BUTTON_SAMPLE_TIMES 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct button_driver_t button_driver_t; /*!< Type of button object */

struct button_driver_t {
    /*!< (optional) Need Support Power Save */
    bool enable_power_save;

    /*!< (necessary) Get key level */
    uint8_t (*get_key_level)(button_driver_t *button_driver);

    /*!< (optional) Enter Power Save cb */
    esp_err_t (*enter_power_save)(button_driver_t *button_driver);

    /*!< (optional) Del the hardware driver and cleanup */
    esp_err_t (*del)(button_driver_t *button_driver);
};

#ifdef __cplusplus
}
#endif
