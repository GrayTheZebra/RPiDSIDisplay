// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int sda, scl, ldo_channel;
    uint8_t address, brightness;
    uint16_t width, height, hsync, hbp, hfp, vsync, vbp, vfp;
    float lane_mbps, pixel_mhz;
    i2c_master_bus_handle_t i2c_bus;
    bool use_vtg;
} rpi_dsi_config_t;
esp_err_t rpi_dsi_backend_begin(const rpi_dsi_config_t *cfg, uint8_t **pixels);
esp_err_t rpi_dsi_backend_flush(void);
esp_err_t rpi_dsi_backend_brightness(uint8_t value);
esp_err_t rpi_dsi_backend_end(void);
#ifdef __cplusplus
}
#endif
