# Testing

The standalone grid was confirmed on hardware without visible offset using
the 0.2.0 default timings. Packaged-library hardware results are still pending.

## Build

Install Arduino CLI and run from this repository root:

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.12 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "Adafruit GFX Library@1.12.6" "Adafruit BusIO@1.17.4"
for sketch in examples/*; do
  arduino-cli compile --fqbn esp32:esp32:esp32p4:PSRAM=enabled,ChipVariant=postv3 --library . "$sketch"
done
```

## Hardware acceptance

1. BoundaryTest: four 8-pixel colored borders fully visible, four corner markers,
   straight 50-pixel grid, centered cross; compare to the standalone reference.
2. GraphicsTest: readable text in all four orientations, correctly oriented RGB
   blocks and complete shapes; moving square does not leave persistent trails.
   Single-buffer tearing is a documented limitation.
3. Power-cycle several times and confirm repeatable startup.
4. HelloDisplay / ColorBars / LiveValues: no start or flush error.

Record core, board/chip revision, panel, settings, example and outcome. A
successful compile is not a substitute for physical display validation.

## Local checks for 0.2.0

Host tests: 1,156 clipped rectangle cases across all four rotations, framebuffer
canaries, primary color conversion, flush forwarding, instance ownership and
retry after failed begin. Passed under AddressSanitizer/UndefinedBehaviorSanitizer;
LeakSanitizer disabled locally because the sandbox cannot inspect process tasks.
Run `bash tests/run.sh` on Linux with a C++17 compiler.
The test uses minimal GFX/backend doubles and does not validate ESP-IDF code.

Full Arduino build was attempted but blocked while downloading package indexes
(network access from Arduino CLI unavailable). The CI workflow is prepared,
not yet run on GitHub. No hardware test of this packaged release has been run.
