#include <RPiDSIDisplay.h>

RPiDSIDisplay display;

void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.println(display.lastErrorName());
    return;
  }
  display.setBrightness(180);
  display.fillScreen(0);
  // Match the standalone pattern exactly, now through the GFX interface.
  for (int x = 0; x < 800; x += 50)
    display.drawFastVLine(x, 0, 480, display.color565(48, 48, 48));
  for (int y = 0; y < 480; y += 50)
    display.drawFastHLine(0, y, 800, display.color565(48, 48, 48));
  display.fillRect(398, 0, 4, 480, 0xffff);
  display.fillRect(0, 238, 800, 4, 0xffff);
  for (int x = 0; x < 800; x += 10)
    display.drawFastVLine(x, 40, x % 50 == 0 ? 20 : 8, 0xffff);
  display.fillRect(16, 16, 8, 8, 0xffff);
  display.fillRect(776, 16, 8, 8, 0xffff);
  display.fillRect(16, 456, 8, 8, 0xffff);
  display.fillRect(776, 456, 8, 8, 0xffff);
  display.fillRect(0, 0, 8, 480, 0x07e0);
  display.fillRect(792, 0, 8, 480, 0xffe0);
  display.fillRect(0, 0, 800, 8, 0xf800);
  display.fillRect(0, 472, 800, 8, 0x001f);
  if (!display.flushChecked()) Serial.println(display.lastErrorName());
  Serial.println("BoundaryTest: 20 MHz / 480 Mbps / H=2,46,23 / VTG=ON");
}

void loop() { yield(); }
