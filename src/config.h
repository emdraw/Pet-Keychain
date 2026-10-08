#pragma once
// Hardware and behaviour settings. See docs/WIRING.md for the wiring these pins assume.

// ---- Pins ------------------------------------------------------------------
#define PIN_TFT_SCK   4
#define PIN_TFT_MOSI  6
#define PIN_TFT_DC    5
#define PIN_TFT_RST   3
#define PIN_TFT_CS   -1   // 7-pin ST7789 modules have no CS pin
#define PIN_TFT_BL   10   // backlight
#define PIN_BTN       1   // TTP223 output (active HIGH). The ESP32-C3 has no built-in touch sensor
#define PIN_BAT_ADC   0   // ADC1 pin (GPIO0-4) fed by a 2:1 divider from the battery; -1 = battery simulated from the web UI
#define BAT_DIV_RATIO 2.0f  // (R1 + R2) / R2 of that divider
#define BAT_MV_EMPTY  3300
#define BAT_MV_FULL   4150

// ---- Display ---------------------------------------------------------------
#define TFT_INVERT        true  // colours look inverted? set to false
#define TFT_OFFSET_X      0
#define TFT_OFFSET_Y      0     // image shifted by 80 px? set to 80
#define TFT_SPI_MODE      3     // modules without CS use mode 3; with a CS pin try 0

// ---- Behaviour -------------------------------------------------------------
#define SHOW_CLOCK        true  // draw the time (needs NTP or a time set from the web UI)
#define STATUS_TIMEOUT_MS 10000 // Status/Network tabs return to the face after this many ms
#define LONG_PRESS_MS     4000  // holding the button this long opens WiFi setup mode
#define WIFI_TRY_MS       35000 // after boot, if no saved network connects within this time, open setup mode
#define MAX_NETS          6     // saved WiFi networks
#define AP_SSID           "Llavero-Setup"  // access point used by the setup mode
#define AP_PASS           "llavero1234"    // its password (also encoded in the on-screen QR); change it if you like
#define TIMEZONE          "CST6"           // POSIX TZ string, e.g. "CET-1CEST,M3.5.0,M10.5.0/3", "EST5EDT,M3.2.0,M11.1.0"
