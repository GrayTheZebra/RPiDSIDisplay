#include <RPiDSIDisplay.h>

RPiDSIDisplay display;
bool started = false;
uint8_t rotationIndex = 0;
uint32_t lastRotation = 0, lastFrame = 0;
int previousX = -1;

void drawPage() {
  display.setRotation(rotationIndex);
  display.fillScreen(0);
  const int w = display.width(), h = display.height();
  display.drawRect(0, 0, w, h, 0xffff);
  display.setTextWrap(false);
  display.setTextColor(0xffff);
  display.setTextSize(3);
  display.setCursor(20, 20);
  display.print("Rotation "); display.println(rotationIndex);
  display.setTextSize(2);
  display.setCursor(20, 60);
  display.print(w); display.print(" x "); display.println(h);
  display.setCursor(20, 90);
  display.println("AaZz 0123456789 !?+-");
  display.fillRect(20, 130, 80, 50, 0xf800);
  display.fillRect(110, 130, 80, 50, 0x07e0);
  display.fillRect(200, 130, 80, 50, 0x001f);
  display.drawCircle(w / 2, h / 2, 50, 0xffe0);
  display.drawLine(20, h - 120, w - 20, h - 120, 0x07ff);
  previousX = -1;
}

void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.println(display.lastErrorName());
    return;
  }
  display.setBrightness(180);
  started = true;
  lastRotation = millis();
  drawPage();
}

void loop() {
  if (!started) { yield(); return; }
  const uint32_t now = millis();
  if ((uint32_t)(now - lastRotation) >= 6000) {
    lastRotation = now;
    rotationIndex = (rotationIndex + 1) % 4;
    drawPage();
  }
  if ((uint32_t)(now - lastFrame) >= 50) {
    lastFrame = now;
    const int y = display.height() - 70;
    if (previousX >= 0) display.fillRect(previousX, y, 24, 24, 0);
    const int x = 20 + ((now - lastRotation) / 20) % (display.width() - 64);
    display.fillRect(x, y, 24, 24, 0xf81f);
    previousX = x;
    if (!display.flushChecked()) Serial.println(display.lastErrorName());
  }
  yield();
}
