# Changelog

## 0.2.0 — 2026-09-30

- Use standalone-confirmed defaults: 20 MHz / 480 Mbps, H=2/46/23,
  V=2/21/7, 800×480 and VTG enabled.
- Retain ordered SYSCTRL/timing/VFUEN/LCDCTRL initialization.
- Remove experimental bridge readback and unconditional diagnostic output.
- Preserve Print-compatible void flush() and bool flushChecked().
- Match standalone pattern in BoundaryTest; add GraphicsTest.
- Add repository documentation, contribution/issue guidance and compile CI.
- Packaged-library hardware acceptance remains pending.

## 0.1.x — development diagnostics

Initial GFX wrapper, timing experiments and optional readback. These versions
were diagnostic snapshots; the readback path did not work on the tested setup.
