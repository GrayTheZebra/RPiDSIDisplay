#include <RPiDSIDisplay.h>
RPiDSIDisplay display;
bool displayOK = false;
uint32_t lastUpdate = 0;
bool reversed = false;
void paintBars() {
  const uint16_t colors[] = {0xffff, 0xffe0, 0x07ff, 0x07e0, 0xf81f, 0xf800, 0x001f, 0};
  for (int i = 0; i < 8; ++i) {
    int x0 = i * display.width() / 8;
    int x1 = (i + 1) * display.width() / 8;
    display.fillRect(x0, 0, x1-x0, display.height(), colors[reversed ? 7-i : i]);
  }
  if (!display.flushChecked()) Serial.println(display.lastErrorName());
}
void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.printf("Display: %s\n", display.lastErrorName());
    return;
  }
  displayOK = true;
  paintBars();
  lastUpdate = millis();
}
void loop() {
  if (displayOK && (uint32_t)(millis()-lastUpdate) >= 5000) {
    lastUpdate = millis();
    reversed = !reversed;
    paintBars();
  }
  yield();
}
