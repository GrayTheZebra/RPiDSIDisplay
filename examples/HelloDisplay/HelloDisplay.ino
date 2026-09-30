#include <RPiDSIDisplay.h>
RPiDSIDisplay display;
bool displayOK = false;
void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.printf("Display: %s\n", display.lastErrorName());
    return;
  }
  displayOK = true;
  display.setBrightness(180);
  display.fillScreen(0x0000);
  display.setTextColor(0xffff);
  display.setTextSize(4);
  display.setCursor(30, 35);
  display.println("RPi DSI + Arduino");
  display.drawRoundRect(20, 110, 760, 200, 16, 0xf800);
  display.setTextSize(3);
  display.setCursor(45, 150);
  display.println("800 x 480 - ESP32-P4");
  display.setCursor(45, 215);
  display.setTextColor(0x07e0);
  display.println("Adafruit GFX ist bereit.");
  if (!display.flushChecked()) Serial.println(display.lastErrorName());
}
void loop() { yield(); }
