# RPiDSIDisplay

Arduino library for the original **800×480 Raspberry Pi DSI display** with
**TC358762 bridge and ATTINY controller**, on **ESP32-P4**. Provides an
Adafruit GFX interface backed by an RGB888 framebuffer. Touch is not included.
This is not a universal driver for arbitrary DSI panels.

## Status and supported setup

Version **0.2.0** targets **Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5**.
The standalone driver with the same initialization and default timings was
confirmed on real hardware without visible offset on 2026-09-30:
Waveshare ESP32-P4-WIFI6-DEV-KIT Rev.1.2, P4 revision 3.1 / post-v3,
32 MB PSRAM, original Raspberry Pi 800×480 display.

The packaged 0.2.0 library still needs the BoundaryTest and GraphicsTest
hardware checks. Do not interpret the standalone result as validation on every
panel or as a completed hardware test of all GFX operations.
The driver accesses a private ESP-IDF DSI bus layout: version upgrades require
review. Other IDF major/minor versions are rejected during compilation.

## Installation

1. Install **esp32 by Espressif Systems 3.3.12** in Arduino Boards Manager.
2. Select your ESP32-P4 board, correct chip revision (post-v3 for the tested
   board), and enable PSRAM. Keep the known working board settings.
3. Install **Adafruit GFX Library** and its Adafruit BusIO dependency.
4. Install this repository ZIP via **Sketch → Include Library → Add .ZIP Library**.
   Replace any older RPiDSIDisplay installation instead of keeping duplicates.
5. Open **File → Examples → RPiDSIDisplay → BoundaryTest**.
6. Upload, then power-cycle both ESP and display. Serial monitor: 115200 baud.

Use the board's DSI connector and the existing correct panel power wiring.
Default ATTINY control uses SDA GPIO 7, SCL GPIO 8, address 0x45. The internal
MIPI PHY supply uses LDO channel 3 at 2.5 V; this is not the panel power input.

## Quick start

```cpp
#include <RPiDSIDisplay.h>
RPiDSIDisplay display;

void setup() {
  Serial.begin(115200);
  if (!display.begin()) {
    Serial.println(display.lastErrorName());
    return;
  }
  display.setBrightness(180);
  display.fillScreen(0x0000);
  display.setTextColor(0xffff);
  display.setTextSize(3);
  display.setCursor(20, 20);
  display.println("Hello DSI!");
  if (!display.flushChecked()) Serial.println(display.lastErrorName());
}

void loop() { yield(); }
```

## Default timings

| Setting | Default |
| --- | --- |
| Native resolution | 800 × 480 |
| Pixel clock | 20 MHz |
| DSI data lanes / bitrate | 1 / 480 Mbps |
| HSYNC / back porch / front porch | 2 / 46 / 23 pixel clocks |
| VSYNC / back porch / front porch | 2 / 21 / 7 lines |
| TC358762 timing generator (`useVtg`) | enabled |
| Brightness | 255 |

The total is 871 × 510 clocks per frame, approximately 45 Hz. These are
empirically verified values for the tested setup, not universal panel timings.
HBP=46 must be paired with HFP=23 for this baseline; the earlier HFP=1 test
was not equivalent and produced image corruption.
Continuous HS clock, non-burst sync-pulse video, disabled LP video intervals,
and disabled command/frame ACK are retained from the tested standalone setup.

Configure with `RPiDSIConfig config;` and `display.begin(config)`.
Fields: `sda`, `scl`, `i2cAddress`, `brightness`, `width`, `height`, `hsync`,
`hbp`, `hfp`, `vsync`, `vbp`, `vfp`, `pixelMHz`, `laneMbps`, `useVtg`,
`ldoChannel`, `i2cBus`. Changing dimensions does not make a different panel
compatible automatically. Use `ldoChannel=-1` only when the required PHY
supply is already managed externally.

## API

| Method | Purpose |
| --- | --- |
| `begin(config)` / `begin()` | Initialize; returns bool |
| `end()` | Release resources; returns bool |
| `ready()` | Framebuffer available |
| GFX drawing and text methods | RGB565 drawing colors |
| `setRotation(0..3)` | Software rotation; width/height follow rotation |
| `flush()` | Synchronize framebuffer cache; void, compatible with Print |
| `flushChecked()` | Same synchronization, with bool result |
| `setBrightness(0..255)` | Backlight; returns bool |
| `lastError()` / `lastErrorName()` | Last operation error |
| `getFramebuffer()` / `framebufferSize()` | Native raw buffer access |
| `color565(r,g,b)` | Convert RGB888 values to a GFX color |

Call flush after drawing; drawing does not automatically synchronize the cache.
The raw buffer stores **B, G, R bytes**, 3 bytes per pixel, native orientation,
2400-byte row stride at 800 pixels. It uses 1,152,000 bytes at 800×480.
Raw writes do not apply GFX rotation. Pointers become invalid on `end()`.
One framebuffer is scanned continuously: flush is not a VSync barrier and
updates are not atomic. Animation may exhibit tearing.
Only one active instance is supported. Do not call the library concurrently
from multiple tasks. Failed cleanup can be retried with `end()`.
Initialization/power sequencing requires delays; runtime examples use millis().

## Shared I2C

`config.i2cBus` accepts an existing **new ESP-IDF master-driver handle**, not a
TwoWire/Wire object. Keep it alive until after end(). The library removes only
its ATTINY device, not the external bus. Do not independently initialize Wire
on the same pins. ATTINY reads use separate address/write and read transfers
because the controller does not tolerate a repeated start in this sequence.
Shared Wire use is not validated.

## Examples and validation

- **BoundaryTest**: matches the standalone grid, scale, colored edges and corners.
- **GraphicsTest**: text, RGB blocks, shapes, all four rotations every 6 seconds,
  and a moving square at a 50 ms update interval.
- **HelloDisplay**: basic text and shapes.
- **ColorBars**: periodically reversed colors.
- **LiveValues**: explicitly simulated values, no sensor input.
- **CustomConfig**: custom pins/brightness and portrait orientation.

The confirmed standalone reference is preserved in `extras/DSI_Standalone`.

See [TESTING.md](TESTING.md) for build commands and hardware acceptance checks.
GitHub Actions compiles all examples for the pinned ESP32 core.
See [CHANGELOG.md](CHANGELOG.md) for the changes from diagnostic versions.

## Troubleshooting

- **Offset/corruption**: run BoundaryTest with defaults and include a whole-panel
  photo, core version, board revision, configuration and serial log.
- **No backlight**: inspect begin()'s error, panel supply, control pins and ID.
- **Old flush compilation error**: remove duplicate/older libraries; use
  flushChecked() when checking success.
- **Unsupported IDF version**: install the target core; do not simply remove
  the compile guard because private structures can change.
- Diagnostic versions 0.1.x exposed `diagnosticReadback`; that experimental
  option has been removed. Delete it from older sketches.

## License and provenance

**GPL-2.0-only**, see [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).
The backend derives from Kuro's esp_lcd_rpi_dsi 0.1.0; original copyright
notices are retained. Adafruit GFX and ESP-IDF remain external dependencies
under their respective licenses. See [CONTRIBUTING.md](CONTRIBUTING.md).

## Publishing this repository

Upload the contents of this folder to the root of a repository named
`GrayTheZebra/RPiDSIDisplay` (the target URL in library.properties), or update
that URL if you choose another repository. Enable GitHub Actions, check the
build result, complete the hardware checklist, then create tag `v0.2.0` and a
release. This package does not create a GitHub repository or publish a release.
Arduino Library Manager submission is a separate step.
