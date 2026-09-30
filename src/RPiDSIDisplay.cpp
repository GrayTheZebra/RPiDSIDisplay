// SPDX-License-Identifier: GPL-2.0-only
#include "RPiDSIDisplay.h"

RPiDSIDisplay *RPiDSIDisplay::owner_ = nullptr;
RPiDSIDisplay::RPiDSIDisplay() : Adafruit_GFX(800, 480) {}
RPiDSIDisplay::~RPiDSIDisplay() { end(); }

bool RPiDSIDisplay::begin(const RPiDSIConfig &c) {
    if (owner_) { error_ = ESP_ERR_INVALID_STATE; return false; }
    rpi_dsi_config_t cfg = {};
    cfg.sda = c.sda; cfg.scl = c.scl; cfg.ldo_channel = c.ldoChannel;
    cfg.address = c.i2cAddress; cfg.brightness = c.brightness;
    cfg.width = c.width; cfg.height = c.height;
    cfg.hsync = c.hsync; cfg.hbp = c.hbp; cfg.hfp = c.hfp;
    cfg.vsync = c.vsync; cfg.vbp = c.vbp; cfg.vfp = c.vfp;
    cfg.lane_mbps = c.laneMbps; cfg.pixel_mhz = c.pixelMHz;
    cfg.i2c_bus = c.i2cBus;
    cfg.use_vtg = c.useVtg;
    owner_ = this;
    error_ = rpi_dsi_backend_begin(&cfg, &pixels_);
    if (error_ != ESP_OK) {
        pixels_ = nullptr;
        if (rpi_dsi_backend_end() == ESP_OK) owner_ = nullptr;
        return false;
    }
    WIDTH = c.width; HEIGHT = c.height;
    setRotation(0);
    setCursor(0, 0);
    return true;
}

bool RPiDSIDisplay::end() {
    if (owner_ != this) return true;
    pixels_ = nullptr;
    error_ = rpi_dsi_backend_end();
    if (error_ == ESP_OK) owner_ = nullptr;
    return error_ == ESP_OK;
}

void RPiDSIDisplay::flush() {
    (void)flushChecked();
}

bool RPiDSIDisplay::flushChecked() {
    error_ = ready() ? rpi_dsi_backend_flush() : ESP_ERR_INVALID_STATE;
    return error_ == ESP_OK;
}
bool RPiDSIDisplay::setBrightness(uint8_t value) {
    error_ = ready() ? rpi_dsi_backend_brightness(value) : ESP_ERR_INVALID_STATE;
    return error_ == ESP_OK;
}

void RPiDSIDisplay::rawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) {
    uint8_t r = (color >> 11) & 31, g = (color >> 5) & 63, b = color & 31;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    for (int32_t row = y; row < y + h; ++row) {
        uint8_t *p = pixels_ + ((size_t)row * WIDTH + x) * 3;
        for (int32_t col = 0; col < w; ++col) {
            *p++ = b; *p++ = g; *p++ = r;
        }
    }
}

void RPiDSIDisplay::drawPixel(int16_t x, int16_t y, uint16_t color) {
    fillRect(x, y, 1, 1, color);
}
void RPiDSIDisplay::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!pixels_ || w <= 0 || h <= 0) return;
    int32_t x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int32_t x1 = (int32_t)x + w, y1 = (int32_t)y + h;
    if (x1 > _width) x1 = _width;
    if (y1 > _height) y1 = _height;
    if (x0 >= x1 || y0 >= y1) return;
    int32_t rw = x1 - x0, rh = y1 - y0;
    switch (rotation) {
        case 1: rawRect(WIDTH - y1, x0, rh, rw, color); break;
        case 2: rawRect(WIDTH - x1, HEIGHT - y1, rw, rh, color); break;
        case 3: rawRect(y0, HEIGHT - x1, rh, rw, color); break;
        default: rawRect(x0, y0, rw, rh, color); break;
    }
}
void RPiDSIDisplay::fillScreen(uint16_t color) {
    if (pixels_) rawRect(0, 0, WIDTH, HEIGHT, color);
}
void RPiDSIDisplay::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    fillRect(x, y, w, 1, color);
}
void RPiDSIDisplay::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    fillRect(x, y, 1, h, color);
}
