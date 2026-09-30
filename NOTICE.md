# Provenance

Backend: adapted from Kuro, esp_lcd_rpi_dsi 0.1.0 (GPL-2.0-only).
https://components.espressif.com/components/kurozz/esp_lcd_rpi_dsi/versions/0.1.0
Original SPDX copyright notices are preserved in src/rpi_dsi_backend.c.

ATTINY register/power sequencing follows the Linux Raspberry Pi panel and
ATTINY regulator drivers. TC358762 register definitions and timing upload
follow Tomi Valkeinen's Linux drm/bridge tc358762 patch series, 2026-09-07:
https://mail-archive.com/dri-devel%40lists.freedesktop.org/msg633476.html
https://mail-archive.com/dri-devel%40lists.freedesktop.org/msg633482.html
https://mail-archive.com/dri-devel%40lists.freedesktop.org/msg633480.html

This adaptation adds Arduino configuration, resource ownership, command-ACK
handling, Adafruit GFX integration and examples. Modifications are distributed
under GPL-2.0-only. External dependencies are not bundled.
