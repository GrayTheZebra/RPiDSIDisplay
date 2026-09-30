# DSI Standalone

Eigenständiger Arduino-Sketch für ESP32-P4 / Arduino-ESP32 3.3.12 (IDF 5.5.5).
Keine Installation von RPiDSIDisplay oder Adafruit GFX erforderlich.
Alle Dateien im selben Sketch-Ordner lassen und DSI_Standalone.ino öffnen.
Die .c-Datei wird von Arduino automatisch mitkompiliert.

Boardeinstellungen und Anschlüsse wie bisher: PSRAM an, SDA 7, SCL 8,
ATTINY 0x45, MIPI-LDO 3. Keine Änderungen an der Verkabelung.
Serieller Monitor: 115200 Baud.

Erster Test: test_config.h unverändert lassen. Die Werte entsprechen dem letzten
sichtbaren Bibliotheksstand: 20 MHz / 480 Mbps, HSYNC/HBP/HFP 2/46/23, VTG an.
Die Initialisierung entspricht 0.1.1; die fehlgeschlagene Rücklesung ist entfernt.
Das Bild wird direkt als B,G,R-Bytes in den RGB888-Framebuffer geschrieben.
Es gibt keine GFX-Rotation und keinen Koordinatenoffset.

Erwartetes Bild: roter oberer und blauer unterer Rand, grün links, gelb rechts;
alle Ränder 8 Pixel breit. Weißes Mittelkreuz und vier kleine Eckmarkierungen.
Dunkles 50-Pixel-Raster und eine Skala ab x=0 in 10-Pixel-Schritten bei y=40.
Bitte ein Foto des gesamten Displays liefern, möglichst in Leserichtung.

Dieser Stand mit HBP=46 und HFP=23 wurde vom Nutzer am 2026-09-30
auf dem Display ohne sichtbare Verschiebung bestätigt.
GPL-2.0-only; ursprünglicher Treiber Copyright 2026 Kuro, siehe LICENSE.
