#include <RPiDSIDisplay.h>
RPiDSIDisplay display;
void setup() {
  Serial.begin(115200);
  RPiDSIConfig config;
  config.sda = 7; config.scl = 8;
  config.brightness = 160;
  // config.i2cBus = existingBus; // IDF new master driver handle, not Wire.
  // config.ldoChannel = -1;     // Only if 2.5 V PHY supply is managed externally.
  if (!display.begin(config)) {
    Serial.println(display.lastErrorName());
    return;
  }
  display.setRotation(1);
  display.fillScreen(0);
  display.setCursor(20, 20);
  display.setTextSize(3);
  display.setTextColor(0xffff);
  display.println("Hochformat 480x800");
  display.flush();
}
void loop() { yield(); }
