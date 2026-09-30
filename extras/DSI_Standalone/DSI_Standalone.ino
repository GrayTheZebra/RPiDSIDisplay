#include <Arduino.h>
#include "rpi_display.h"
#include "test_config.h"

void setup() {
  Serial.begin(115200);
  Serial.printf("\nDSI_STANDALONE: pixel=%d MHz lane=%d Mbps H=%d/%d/%d VTG=%d\n",
                TEST_PIXEL_MHZ, TEST_LANE_MBPS, TEST_HSYNC,
                TEST_HBP, TEST_HFP, TEST_VTG);
  if (!ESP.getPsramSize()) {
    Serial.println("PSRAM fehlt. Bitte in den Board-Einstellungen aktivieren.");
    return;
  }
  esp_err_t err = rpi_display_begin();
  if (err != ESP_OK) {
    Serial.printf("Startfehler: %s\n", esp_err_to_name(err));
    return;
  }
  Serial.println("Testbild aktiv: oben ROT, unten BLAU, links GRUEN, rechts GELB.");
  Serial.println("Rand: 8 Pixel. Raster: 50 Pixel. Skala oben: 10 Pixel pro Strich.");
}

void loop() { yield(); }
