// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <Adafruit_GFX.h>
#include "rpi_dsi_backend.h"

struct RPiDSIConfig {
    int sda = 7, scl = 8;
    int ldoChannel = 3; // -1: PHY supply already managed externally
    uint8_t i2cAddress = 0x45, brightness = 255;
    uint16_t width = 800, height = 480;
    uint16_t hsync = 2, hbp = 46, hfp = 23;
    uint16_t vsync = 2, vbp = 21, vfp = 7;
    float laneMbps = 480, pixelMHz = 20;
    bool useVtg = true; // TC358762 timing generator.
    // Optional IDF NEW master-driver bus. Not a TwoWire/Wire object.
    i2c_master_bus_handle_t i2cBus = nullptr;
};

class RPiDSIDisplay : public Adafruit_GFX {
public:
    RPiDSIDisplay();
    ~RPiDSIDisplay();
    RPiDSIDisplay(const RPiDSIDisplay &) = delete;
    RPiDSIDisplay &operator=(const RPiDSIDisplay &) = delete;
    bool begin(const RPiDSIConfig &config = RPiDSIConfig());
    bool end();
    bool ready() const { return pixels_ != nullptr; }
    void flush() override;
    bool flushChecked();
    bool setBrightness(uint8_t value);
    esp_err_t lastError() const { return error_; }
    const char *lastErrorName() const { return esp_err_to_name(error_); }
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
    void fillScreen(uint16_t color) override;
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
    // Raw B,G,R byte order, native orientation; call flush() after edits.
    uint8_t *getFramebuffer() { return pixels_; }
    size_t framebufferSize() const { return pixels_ ? (size_t)WIDTH * HEIGHT * 3 : 0; }
    static uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
    }
private:
    static RPiDSIDisplay *owner_;
    uint8_t *pixels_ = nullptr;
    esp_err_t error_ = ESP_OK;
    void rawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
};
