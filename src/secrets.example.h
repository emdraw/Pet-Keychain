#pragma once
// Template for src/secrets.h (which is git-ignored). Copy this file to secrets.h and edit it:
//   cp src/secrets.example.h src/secrets.h
//
// If src/secrets.h does not exist the firmware still builds with these empty defaults:
// the keychain simply starts in setup mode and you add your WiFi from your phone (QR screen).

// Optional: a WiFi network to remember on the very first boot. Leave empty to use the QR setup instead.
#define WIFI_SSID ""
#define WIFI_PASS ""

// Name on the network (http://llavero.local) and password for over-the-air updates.
// CHANGE OTA_PASS: anyone on your network who knows it can flash new firmware.
#define OTA_HOSTNAME "llavero"
#define OTA_PASS "change-me"
