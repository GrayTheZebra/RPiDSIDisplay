#include <RPiDSIDisplay.h>
RPiDSIDisplay display;
bool displayOK = false;
uint32_t lastUpdate = 0;
void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.printf("Display: %s\n", display.lastErrorName());
    return;
  }
  displayOK = true;
  display.fillScreen(0);
  display.setTextSize(4);
  display.setTextColor(0xffff);
  display.setCursor(25, 25);
  display.println("Messwerte - DEMO");
  display.setTextSize(2);
  display.setCursor(25, 90);
  display.println("Simulierte Werte, kein Mikrofon angeschlossen");
  display.flush();
}
void loop() {
  uint32_t now = millis();
  if (displayOK && (uint32_t)(now-lastUpdate) >= 200) {
    lastUpdate = now;
    unsigned value = (now / 100) % 101;
    display.fillRect(25, 150, 750, 150, 0);
    display.setCursor(25, 155);
    display.setTextSize(4);
    display.setTextColor(0x07e0);
    display.print("Pegel: "); display.print(value); display.println(" %");
    display.drawRect(25, 230, 750, 50, 0xffff);
    display.fillRect(27, 232, value * 746 / 100, 46, 0xf800);
    if (!display.flushChecked()) Serial.println(display.lastErrorName());
  }
  yield();
}
