/*
 * SPDX-FileCopyrightText: 2026 Kuro
 * SPDX-License-Identifier: GPL-2.0-only
 * Arduino adaptation of kurozz/esp_lcd_rpi_dsi 0.1.0.
 * Target: Arduino-ESP32 3.3.12 / ESP-IDF 5.5.x, ESP32-P4.
 */
#include "rpi_display.h"
#include "test_config.h"
#include <stdio.h>
#include "esp_idf_version.h"
#include "soc/soc_caps.h"
#if !SOC_MIPI_DSI_SUPPORTED
#error "Select an ESP32-P4 board"
#endif
#if ESP_IDF_VERSION_MAJOR != 5 || ESP_IDF_VERSION_MINOR != 5
#error "This driver targets ESP-IDF 5.5.x (Arduino-ESP32 3.3.12)"
#endif
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_cache.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_mipi_dsi.h"
#include "driver/i2c_master.h"
#include "hal/mipi_dsi_hal.h"
#include "hal/mipi_dsi_host_ll.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static const char *TAG = "rpi_dsi";
/* ATTINY88 register map, from panel-raspberrypi-touchscreen.c */
#define REG_ID      0x80
#define REG_PORTA   0x81
#define REG_PORTB   0x82
#define REG_PORTC   0x83
#define REG_POWERON 0x85 // revision-1 firmware only, unused here
#define REG_PWM     0x86
#define REG_ADDR_L  0x8c
#define REG_ADDR_H  0x8d
#define REG_WRITE_DATA_H 0x90
#define REG_WRITE_DATA_L 0x91

/* ATTINY firmware revisions, from the REG_ID check in
 * drivers/regulator/rpi-panel-attiny-regulator.c */
#define ATTINY_FW_V1 0xde
#define ATTINY_FW_V2 0xc3

/* Port bits. PORTA/PORTB/PORTC are write-only: the Linux driver keeps a shadow
 * copy in state->port_states[] and never reads them back from the device, so
 * there is no status here to poll -- only the delays below. */
#define PA_LCD_LR       (1 << 2) // orientation the closed-source firmware uses
#define PB_LCD_VCC_N    (1 << 1)
#define PB_LCD_MAIN     (1 << 7)
#define PC_LED_EN       (1 << 0)
#define PC_RST_TP_N     (1 << 1) // touch controller reset, active low
#define PC_RST_LCD_N    (1 << 2) // LCD reset, active low
#define PC_RST_BRIDGE_N (1 << 3) // TC358762 reset, active low

/* TC358762 registers, from the same driver */
#define PPI_STARTPPI         0x0104
#define PPI_LPTXTIMECNT      0x0114
#define PPI_D0S_ATMR         0x0144
#define PPI_D1S_ATMR         0x0148
#define PPI_D0S_CLRSIPOCOUNT 0x0164
#define PPI_D1S_CLRSIPOCOUNT 0x0168
#define DSI_STARTDSI         0x0204
#define DSI_LANEENABLE       0x0210
#define LCDCTRL              0x0420
#define SPICMR               0x0450
#define SYSCTRL              0x0464

#define DSI_LANEENABLE_CLOCK (1 << 0)
#define DSI_LANEENABLE_D0    (1 << 1)

/* Same initialization as library 0.1.1, without its graphics layer. */
typedef struct {
    uint16_t reg;
    uint32_t val;
    uint16_t delay_ms;
} rpi_dsi_bridge_cmd_t;

static const rpi_dsi_bridge_cmd_t s_bridge_init[] = {
    { SYSCTRL, 0x040f, 100 },
    { DSI_LANEENABLE,       DSI_LANEENABLE_CLOCK | DSI_LANEENABLE_D0, 0 },
    { PPI_D0S_CLRSIPOCOUNT, 0x05,       0 },
    { PPI_D1S_CLRSIPOCOUNT, 0x05,       0 },
    { PPI_D0S_ATMR,         0x00,       0 },
    { PPI_D1S_ATMR,         0x00,       0 },
    { PPI_LPTXTIMECNT,      0x03,       0 },
    { SPICMR,               0x00,       0 },
    { 0x0424, TEST_HSYNC | ((uint32_t)TEST_HBP << 16), 0 },
    { 0x0428, 800 | ((uint32_t)TEST_HFP << 16), 0 },
    { 0x042c, 2 | (21UL << 16), 0 },
    { 0x0430, 480 | (7UL << 16), 0 },
    { 0x0434, 1, 0 },
    { LCDCTRL, 0x00100150 | (TEST_VTG ? 2 : 0), 0 },
    { PPI_STARTPPI,         0x01,       0 },
    { DSI_STARTDSI,         0x01,       100 },
};


// Prefix of the private ESP-IDF 5.5 esp_lcd_dsi_bus_t; version-specific.
typedef struct { int bus_id; mipi_dsi_hal_context_t hal; } rpi_bus_prefix_t;
typedef struct {
    mipi_dsi_hal_context_t *hal;
    i2c_master_dev_handle_t attiny;
    uint8_t virtual_channel;
} rpi_dsi_panel_t;
static rpi_dsi_panel_t context;
static esp_ldo_channel_handle_t ldo;
static i2c_master_bus_handle_t i2c_bus;
static esp_lcd_dsi_bus_handle_t dsi_bus;
static esp_lcd_panel_io_handle_t dbi;
static esp_lcd_panel_handle_t panel;
static uint8_t *framebuffer;
static bool attempted;
static esp_err_t rpi_dsi_attiny_write(rpi_dsi_panel_t *ctx, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_transmit(ctx->attiny, buf, sizeof(buf), 100);
}

static esp_err_t rpi_dsi_attiny_read(rpi_dsi_panel_t *ctx, uint8_t reg, uint8_t *val)
{
    // The ATTINY firmware bit-bangs I2C and does not tolerate a repeated start,
    // so address and data go out as two separate transactions, as in Linux.
    ESP_RETURN_ON_ERROR(i2c_master_transmit(ctx->attiny, &reg, 1, 100), TAG, "address write failed");
    esp_rom_delay_us(200);
    return i2c_master_receive(ctx->attiny, val, 1, 100);
}

static void rpi_dsi_bridge_write(rpi_dsi_panel_t *ctx, uint16_t reg, uint32_t val)
{
    // 16-bit register address then 32-bit value, both little endian, in one
    // MIPI DSI generic long write (DT 0x29)
    uint8_t msg[6] = {
        reg & 0xFF, (reg >> 8) & 0xFF,
        val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF,
    };
    mipi_dsi_hal_host_gen_write_long_packet(ctx->hal, ctx->virtual_channel,
                                            MIPI_DSI_DT_GENERIC_LONG_WRITE, msg, sizeof(msg));
}

/* Power sequence from attiny_lcd_power_enable() / attiny_lcd_power_disable().
 * This is the path the maintained Linux driver uses for both firmware
 * revisions; the older REG_POWERON shortcut only exists on revision 1. */
static esp_err_t rpi_dsi_power_on(rpi_dsi_panel_t *ctx)
{
    // hold the bridge, the LCD and the touch controller in reset
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_PORTC, 0), TAG, "PORTC write failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_PORTA, PA_LCD_LR), TAG, "PORTA write failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    // main regulator on, and power to the panel
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_PORTB, PB_LCD_MAIN), TAG, "PORTB write failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_PORTC, PC_LED_EN), TAG, "PORTC write failed");
    vTaskDelay(pdMS_TO_TICKS(80));

    /* Deassert the bridge and LCD resets. In Linux this is NOT part of
     * attiny_lcd_power_enable(): the ATTINY exposes a gpio_chip whose GPIO 0
     * (RST_BRIDGE_N) maps to PC_RST_BRIDGE_N | PC_RST_LCD_N, and the tc358762's
     * vddc-supply is a regulator-fixed hanging off that GPIO, so the release only
     * happens later, when the bridge driver probes. Without it the TC358762 sits
     * in reset and ignores every register write. PC_RST_TP_N rides along: it is
     * GPIO 1, the touch controller's reset, harmless when touch is unused. */
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_PORTC,
                                             PC_LED_EN | PC_RST_BRIDGE_N | PC_RST_LCD_N | PC_RST_TP_N),
                        TAG, "releasing the bridge reset failed");
    vTaskDelay(pdMS_TO_TICKS(8));

    /* The tail of attiny_gpio_set() for RST_BRIDGE_N: the ATTINY has its own
     * back-channel to the bridge's registers, and pokes 0x0000 into bridge
     * register 0x047c on the way out of reset. */
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_ADDR_H, 0x04), TAG, "ADDR_H write failed");
    vTaskDelay(pdMS_TO_TICKS(8));
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_ADDR_L, 0x7c), TAG, "ADDR_L write failed");
    vTaskDelay(pdMS_TO_TICKS(8));
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_WRITE_DATA_H, 0x00), TAG, "DATA_H write failed");
    vTaskDelay(pdMS_TO_TICKS(8));
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_write(ctx, REG_WRITE_DATA_L, 0x00), TAG, "DATA_L write failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

static void rpi_dsi_power_off(rpi_dsi_panel_t *ctx)
{
    rpi_dsi_attiny_write(ctx, REG_PWM, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    rpi_dsi_attiny_write(ctx, REG_PORTA, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    rpi_dsi_attiny_write(ctx, REG_PORTB, PB_LCD_VCC_N);
    vTaskDelay(pdMS_TO_TICKS(10));
    rpi_dsi_attiny_write(ctx, REG_PORTC, 0);
    vTaskDelay(pdMS_TO_TICKS(30));
}


esp_err_t rpi_display_begin(void)
{
    // Call once per boot; a failed initialization requires a restart.
    if (attempted) return ESP_ERR_INVALID_STATE;
    attempted = true;
    esp_ldo_channel_config_t lc = {.chan_id = 3, .voltage_mv = 2500};
    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&lc, &ldo), TAG, "MIPI LDO");
    i2c_master_bus_config_t ic = {
        .i2c_port = -1, .sda_io_num = 7, .scl_io_num = 8,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&ic, &i2c_bus), TAG, "I2C bus");
    i2c_device_config_t ac = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x45, .scl_speed_hz = 100000,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(i2c_bus, &ac, &context.attiny), TAG, "ATTINY device");
    esp_lcd_dsi_bus_config_t bc = {
        .bus_id = 0, .num_data_lanes = 1, .lane_bit_rate_mbps = TEST_LANE_MBPS,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bc, &dsi_bus), TAG, "DSI bus");
    context.hal = &((rpi_bus_prefix_t *)dsi_bus)->hal;
    ESP_RETURN_ON_FALSE(context.hal->host && context.hal->bridge &&
                        context.hal->lane_bit_rate_mbps >= 80 &&
                        context.hal->lane_bit_rate_mbps <= 1500,
                        ESP_ERR_INVALID_STATE, TAG, "Unexpected private DSI layout");
    mipi_dsi_host_ll_set_clock_lane_state(context.hal->host, MIPI_DSI_LL_CLOCK_LANE_STATE_HS);
    esp_lcd_dbi_io_config_t dc = {.virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8};
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(dsi_bus, &dc, &dbi), TAG, "DBI IO");
    // Essential fix: DBI creation enables ACK requests; this bridge stalls with them.
    mipi_dsi_host_ll_enable_cmd_ack(context.hal->host, false);
    esp_lcd_dpi_panel_config_t pc = {
        .virtual_channel = 0, .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = TEST_PIXEL_MHZ,
        .in_color_format = LCD_COLOR_FMT_RGB888,
        .out_color_format = LCD_COLOR_FMT_RGB888,
        .num_fbs = 1,
        .video_timing = {
            .h_size = 800, .v_size = 480,
            .hsync_pulse_width = TEST_HSYNC, .hsync_back_porch = TEST_HBP, .hsync_front_porch = TEST_HFP,
            .vsync_pulse_width = 2, .vsync_back_porch = 21, .vsync_front_porch = 7,
        },
        .flags = {.use_dma2d = true, .disable_lp = true},
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_dpi(dsi_bus, &pc, &panel), TAG, "DPI panel");
    mipi_dsi_host_ll_dpi_set_video_burst_type(context.hal->host,
        MIPI_DSI_LL_VIDEO_NON_BURST_WITH_SYNC_PULSES);
    rpi_dsi_power_off(&context);
    uint8_t id = 0;
    ESP_RETURN_ON_ERROR(rpi_dsi_attiny_read(&context, REG_ID, &id), TAG, "ATTINY read");
    ESP_LOGI(TAG, "ATTINY ID=0x%02x, ACK disabled, 800x480 RGB888", id);
    ESP_RETURN_ON_FALSE(id == ATTINY_FW_V1 || id == ATTINY_FW_V2,
                        ESP_ERR_INVALID_RESPONSE, TAG, "Unexpected ATTINY ID");
    ESP_RETURN_ON_ERROR(rpi_dsi_power_on(&context), TAG, "Panel power");
    mipi_dsi_host_ll_enable_cmd_ack(context.hal->host, false);
    for (size_t i = 0; i < sizeof(s_bridge_init) / sizeof(s_bridge_init[0]); i++) {
        rpi_dsi_bridge_write(&context, s_bridge_init[i].reg, s_bridge_init[i].val);
        if (s_bridge_init[i].delay_ms) vTaskDelay(pdMS_TO_TICKS(s_bridge_init[i].delay_ms));
    }
    mipi_dsi_host_ll_dpi_enable_frame_ack(context.hal->host, false);
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "DPI start");
    mipi_dsi_host_ll_set_clock_lane_state(context.hal->host, MIPI_DSI_LL_CLOCK_LANE_STATE_HS);
    void *fb = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_dpi_panel_get_frame_buffer(panel, 1, &fb), TAG, "Framebuffer");
    framebuffer = (uint8_t *)fb;
    ESP_RETURN_ON_ERROR(rpi_display_pattern(), TAG, "Draw bars");
    return rpi_dsi_attiny_write(&context, REG_PWM, 255);
}

esp_err_t rpi_display_pattern(void)
{
    if (!framebuffer) return ESP_ERR_INVALID_STATE;
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 800; ++x) {
            uint32_t c = 0x000000;
            // Dark grid every 50 pixels; no font or graphics library.
            if (x % 50 == 0 || y % 50 == 0) c = 0x303030;
            if (x >= 398 && x < 402) c = 0xffffff;
            if (y >= 238 && y < 242) c = 0xffffff;
            // Ruler at y=40: one tick every 10 pixels, long ticks every 50.
            if (x % 10 == 0 && y >= 40 && y < (x % 50 == 0 ? 60 : 48))
                c = 0xffffff;
            // Markers centered 20 pixels from each corner.
            if ((x >= 16 && x < 24) || (x >= 776 && x < 784)) {
                if ((y >= 16 && y < 24) || (y >= 456 && y < 464)) c = 0xffffff;
            }
            if (x < 8) c = 0x00ff00;       // left green
            if (x >= 792) c = 0xffff00;    // right yellow
            if (y < 8) c = 0xff0000;       // top red
            if (y >= 472) c = 0x0000ff;    // bottom blue
            size_t i = ((size_t)y * 800 + x) * 3;
            framebuffer[i] = c & 255;
            framebuffer[i + 1] = (c >> 8) & 255;
            framebuffer[i + 2] = (c >> 16) & 255;
        }
    }
    printf("DSI_STANDALONE: frame=800x480, stride=2400, bytes=1152000\n");
    return esp_cache_msync(framebuffer, 800 * 480 * 3,
        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
}
