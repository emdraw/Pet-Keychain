#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <sys/time.h>
#include <time.h>
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"  // builds out of the box; WiFi is then configured from the phone (QR setup)
#endif
#include "config.h"
#include "display.h"
#include "faces.h"
#include "webpage.h"

static LGFX tft;
static LGFX_Sprite canvas(&tft);
static WebServer server(80);
static DNSServer dns;
static Preferences prefs;

// ------------------------------------------------------------ estado general
enum Screen { SC_FACE, SC_STATUS, SC_NET, SC_SETUP };
static Screen screen = SC_FACE;
static uint32_t screenSince = 0, screenTimeout = STATUS_TIMEOUT_MS;

static int curChar = 0;
static int forcedExpr = -1;  // -1 = automatico
static int autoExpr = E_FELIZ;
static uint32_t nextAutoChange = 0;
static uint32_t notifUntil = 0;

static int battery = 100;
static bool batterySim = true;

static Notif notifs[3];
static int nNotifs = 0;

struct SavedNet { String ssid, pass; };
static SavedNet nets[MAX_NETS];
static int nNets = 0;
static bool wifiTrying = false, setupMode = false, ntpStarted = false, timeFromNtp = false;
static bool everConnected = false, wasConn = false, scanning = false, bootShown = false, apCloseDone = true;
static uint32_t wifiTryStart = 0, connectedAt = 0, offlineSince = 0, lastScanAt = 0;
static uint8_t triedMask = 0;
static int tryingIdx = -1;

// ------------------------------------------------------------ utilidades
static String asciiFold(const String& in) {  // la fuente solo tiene ASCII: quita acentos
  static const char* from[] = {"á", "é", "í", "ó", "ú", "ñ", "Á", "É", "Í", "Ó", "Ú", "Ñ", "ü", "¿", "¡"};
  static const char* to[] = {"a", "e", "i", "o", "u", "n", "A", "E", "I", "O", "U", "N", "u", "?", "!"};
  String s = in;
  for (size_t i = 0; i < sizeof(from) / sizeof(from[0]); i++) s.replace(from[i], to[i]);
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t ch = s[i];
    if (ch >= 32 && ch < 127) out += (char)ch;
  }
  return out;
}

static const char* clockText() {
  static char buf[8];
  struct tm tmv;
  if (!getLocalTime(&tmv, 0)) return nullptr;
  snprintf(buf, sizeof buf, "%02d:%02d", tmv.tm_hour, tmv.tm_min);
  return buf;
}

static void setScreen(Screen s, uint32_t timeoutMs = STATUS_TIMEOUT_MS) {
  screen = s;
  screenSince = millis();
  screenTimeout = timeoutMs;
}

static void addNotif(const String& fromRaw, const String& textRaw) {
  static const uint32_t pal[] = {0x2FB36B, 0xE5484D, 0x3D7BFF, 0xF0A020, 0x9B6BFF};
  String from = asciiFold(fromRaw), text = asciiFold(textRaw);
  if (from.isEmpty()) from = "?";
  for (int i = 2; i > 0; i--) notifs[i] = notifs[i - 1];
  Notif& n = notifs[0];
  n = Notif();
  strlcpy(n.from, from.c_str(), sizeof n.from);
  strlcpy(n.text, text.c_str(), sizeof n.text);
  n.initial = toupper(from[0]);
  uint32_t h = 0;
  for (char ch : from) h = h * 31 + ch;
  n.color = pal[h % 5];
  const char* ck = clockText();
  strlcpy(n.time, ck ? ck : "--:--", sizeof n.time);
  if (nNotifs < 3) nNotifs++;
  notifUntil = millis() + 5000;
  setScreen(SC_FACE);
}

// Curva de descarga tipica de una LiPo de 3.7 V (mV -> %)
static int lipoPercent(float mv) {
  static const int T[][2] = {{4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75}, {3950, 70},
                             {3910, 65}, {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45}, {3800, 40}, {3790, 35},
                             {3770, 30}, {3750, 25}, {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5},  {3270, 0}};
  const int n = sizeof(T) / sizeof(T[0]);
  if (mv >= T[0][0]) return 100;
  for (int i = 1; i < n; i++)
    if (mv >= T[i][0]) return T[i][1] + (int)((mv - T[i][0]) * (T[i - 1][1] - T[i][1]) / (T[i - 1][0] - T[i][0]));
  return 0;
}

static int batteryMv = 0;
static bool batteryPresent = false, batteryOver = false;

static void updateBattery() {
  if (PIN_BAT_ADC < 0) return;
  static uint32_t last = 0;
  static float avg = -1;
  if (millis() - last < 200) return;
  last = millis();
  float acc = 0;
  for (int i = 0; i < 8; i++) acc += analogReadMilliVolts(PIN_BAT_ADC);
  float mv = acc / 8 * BAT_DIV_RATIO;
  avg = avg < 0 ? mv : avg * 0.9f + mv * 0.1f;
  batteryMv = (int)avg;
  batteryOver = avg > 5700;  // el pin ve mas de ~2.85 V: ninguna pila con divisor 2:1 da eso -> cableado mal
  batteryPresent = avg >= 2000 && !batteryOver;  // con el pin al aire se lee ~0: sigue valiendo la bateria simulada
  batterySim = !batteryPresent;
  if (batteryPresent) battery = lipoPercent(avg);
}

static int currentExpr() {
  if (forcedExpr >= 0) return forcedExpr;
  if (millis() < notifUntil) return E_NOTIF;
  if (battery < 20) return E_BATERIA;
  return autoExpr;
}

static void updateAutoExpr() {
  if (millis() < nextAutoChange) return;
  int e = random(100) < 6 ? E_AMOR : (random(100) < 60 ? E_FELIZ : E_PARPADEO);  // amor: ~6 % de los cambios
  if (e == E_AMOR && autoExpr == E_AMOR) e = E_FELIZ;
  autoExpr = e;
  nextAutoChange = millis() + (e == E_AMOR ? random(2000, 3500) : random(5000, 12000));  // y dura poco
}

// ------------------------------------------------------------ WiFi
static void applyTz() {
  setenv("TZ", TIMEZONE, 1);
  tzset();
}

// ---- redes guardadas (la mas reciente primero) ----
static void saveNets() {
  prefs.putUChar("wn", nNets);
  for (int i = 0; i < MAX_NETS; i++) {
    String k = String(i);
    if (i < nNets) {
      prefs.putString(("ws" + k).c_str(), nets[i].ssid);
      prefs.putString(("wp" + k).c_str(), nets[i].pass);
    } else {
      prefs.remove(("ws" + k).c_str());
      prefs.remove(("wp" + k).c_str());
    }
  }
}

static void loadNets() {
  if (!prefs.isKey("wn")) {  // primer arranque con esta version: migra la red anterior o la de secrets.h
    String ssid = prefs.getString("ssid", WIFI_SSID);
    if (ssid.length()) {
      nets[0] = {ssid, prefs.getString("pass", WIFI_PASS)};
      nNets = 1;
    }
    saveNets();
    return;
  }
  nNets = min<int>(prefs.getUChar("wn", 0), MAX_NETS);
  for (int i = 0; i < nNets; i++) {
    nets[i].ssid = prefs.getString(("ws" + String(i)).c_str(), "");
    nets[i].pass = prefs.getString(("wp" + String(i)).c_str(), "");
  }
}

static int findNet(const String& ssid) {
  for (int i = 0; i < nNets; i++)
    if (nets[i].ssid == ssid) return i;
  return -1;
}

static void rememberNet(const String& ssid, const String& pass) {  // guarda o actualiza y la pone de primera
  int i = findNet(ssid);
  if (i < 0) i = nNets < MAX_NETS ? nNets++ : MAX_NETS - 1;
  for (int j = i; j > 0; j--) nets[j] = nets[j - 1];
  nets[0] = {ssid, pass};
  saveNets();
}

static void forgetNet(const String& ssid) {
  int i = findNet(ssid);
  if (i < 0) return;
  for (int j = i; j < nNets - 1; j++) nets[j] = nets[j + 1];
  nets[--nNets] = {"", ""};
  saveNets();
}

static void startWifi(const String& ssid, const String& pass) {
  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), pass.c_str());
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  // algunas placas C3 mini fallan con potencia alta
  wifiTrying = true;
  wifiTryStart = millis();
}

static void startSetup(bool show = false) {
  if (setupMode) { if (show) setScreen(SC_SETUP); return; }
  setupMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  dns.start(53, "*", WiFi.softAPIP());
  if (show) setScreen(SC_SETUP);  // por aviso automatico NO se cambia de pantalla: el QR esta en la pestana Red
  Serial.printf("Modo configuracion: red %s, http://%s/\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

static void stopSetup() {
  setupMode = false;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
}

static void wifiLoop() {
  if (setupMode) dns.processNextRequest();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wasConn) {
      wasConn = true;
      wifiTrying = false;
      scanning = false;
      triedMask = 0;
      offlineSince = 0;
      connectedAt = millis();
      everConnected = true;
      Serial.printf("WiFi OK: %s  IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      int i = findNet(WiFi.SSID());
      if (i > 0) { SavedNet n = nets[i]; rememberNet(n.ssid, n.pass); }  // la ultima usada va primero
      if (setupMode) apCloseDone = false;
      if (!ntpStarted) {
        ntpStarted = true;
        configTzTime(TIMEZONE, "pool.ntp.org", "time.google.com");
      }
      if (!setupMode && !bootShown) {  // al encender, muestra la IP unos segundos
        bootShown = true;
        setScreen(SC_NET, 8000);
      }
    }
    // cierra el AP un poco despues de conectar, para que el celular reciba la respuesta
    if (!apCloseDone && millis() - connectedAt > 3000) {
      apCloseDone = true;
      stopSetup();
      bootShown = true;
      setScreen(SC_NET, 30000);
    }
    if (ntpStarted && !timeFromNtp) {
      struct tm tmv;
      if (getLocalTime(&tmv, 0) && millis() - connectedAt > 2000) timeFromNtp = true;
    }
    return;
  }

  // ---- sin conexion: busca la mejor red guardada que se vea ----
  wasConn = false;
  if (!offlineSince) offlineSince = millis();
  if (!setupMode && (nNets == 0 || (!everConnected && millis() - offlineSince > WIFI_TRY_MS))) startSetup();

  if (wifiTrying) {
    if (millis() - wifiTryStart > 12000) {
      if (tryingIdx >= 0) triedMask |= 1 << tryingIdx;
      wifiTrying = false;
    }
    return;
  }
  if (nNets == 0) return;
  if (!scanning) {
    if (setupMode && WiFi.softAPgetStationNum() > 0) return;  // no escanees mientras alguien configura desde el AP
    if (lastScanAt == 0 || millis() - lastScanAt > 3000) {
      WiFi.scanNetworks(true);
      scanning = true;
    }
    return;
  }
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;
  scanning = false;
  lastScanAt = millis();
  if (n < 0) { WiFi.scanDelete(); return; }
  int best = -1, bestRssi = -1000;
  for (int k = 0; k < nNets; k++) {
    if (triedMask & (1 << k)) continue;
    for (int j = 0; j < n; j++)
      if (WiFi.SSID(j) == nets[k].ssid && WiFi.RSSI(j) > bestRssi) { best = k; bestRssi = WiFi.RSSI(j); }
  }
  WiFi.scanDelete();
  if (best >= 0) {
    tryingIdx = best;
    startWifi(nets[best].ssid, nets[best].pass);
  } else if (triedMask) {
    triedMask = 0;  // ya probo todas las visibles: vuelve a intentar en la siguiente ronda
  }
}

// ------------------------------------------------------------ servidor web
static String jsonEsc(const String& s) {
  String o;
  for (char ch : s) {
    if (ch == '"' || ch == '\\') o += '\\';
    o += ch;
  }
  return o;
}

static String stateJson() {
  const char* ck = clockText();
  String s = "{\"c\":\"";
  s += characterAt(curChar).id;
  s += "\",\"e\":" + String(forcedExpr) + ",\"bat\":" + String(battery) + ",\"batSim\":" + (batterySim ? "true" : "false") + ",\"batMv\":" + String(batteryMv) + ",\"batOver\":" + (batteryOver ? "true" : "false");
  s += ",\"n\":" + String(nNotifs) + ",\"time\":\"" + (ck ? ck : "") + "\",\"ntp\":" + (timeFromNtp ? "true" : "false");
  s += ",\"wifi\":{\"conn\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  s += ",\"ssid\":\"" + jsonEsc(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : (nNets ? nets[0].ssid : String())) + "\"";
  s += ",\"ip\":\"" + WiFi.localIP().toString() + "\",\"setup\":" + (setupMode ? "true" : "false") + "}";
  s += ",\"nets\":[";
  for (int i = 0; i < nNets; i++) s += String(i ? "," : "") + "\"" + jsonEsc(nets[i].ssid) + "\"";
  s += "]";
  s += ",\"heap\":" + String(ESP.getFreeHeap()) + ",\"build\":\"" + __DATE__ + __TIME__ + "\"}";
  return s;
}

// Envia el contenido del sprite como BMP de 24 bits (scale 1 = 240x240, 2 = 120x120)
static void sendBmp(int scale, bool cache) {
  const int ow = 240 / scale, oh = 240 / scale;
  const uint32_t dataSize = ow * 3 * oh;
  uint8_t hdr[54] = {'B', 'M'};
  auto put32 = [&](int off, uint32_t v) { for (int i = 0; i < 4; i++) hdr[off + i] = (v >> (8 * i)) & 0xFF; };
  put32(2, 54 + dataSize); put32(10, 54); put32(14, 40); put32(18, ow); put32(22, oh);
  hdr[26] = 1; hdr[28] = 24; put32(34, dataSize);
  server.setContentLength(54 + dataSize);
  if (cache) server.sendHeader("Cache-Control", "max-age=86400");
  else server.sendHeader("Cache-Control", "no-store");
  server.send(200, "image/bmp", "");
  server.sendContent((const char*)hdr, 54);

  const uint16_t* buf = (const uint16_t*)canvas.getBuffer();
  static uint8_t out[1920];
  size_t n = 0;
  const int div = scale * scale;
  for (int y = oh - 1; y >= 0; y--) {  // BMP va de abajo hacia arriba
    for (int x = 0; x < ow; x++) {
      uint32_t r = 0, g = 0, b = 0;
      for (int dy = 0; dy < scale; dy++)
        for (int dx = 0; dx < scale; dx++) {
          uint16_t v = __builtin_bswap16(buf[(y * scale + dy) * 240 + x * scale + dx]);
          uint32_t r5 = v >> 11, g6 = (v >> 5) & 0x3F, b5 = v & 0x1F;
          r += (r5 << 3) | (r5 >> 2);
          g += (g6 << 2) | (g6 >> 4);
          b += (b5 << 3) | (b5 >> 2);
        }
      out[n++] = b / div; out[n++] = g / div; out[n++] = r / div;
    }
    if (n > sizeof(out) - 800) { server.sendContent((const char*)out, n); n = 0; }
  }
  if (n) server.sendContent((const char*)out, n);
}

static void renderNet();
static void renderSetup();

static void setupWeb() {
  // Cara de un personaje tal como se dibuja en la pantalla (sin tocar lo que muestra el TFT)
  server.on("/api/frame", HTTP_GET, []() {
    int i = server.hasArg("c") ? characterIndex(server.arg("c").c_str()) : curChar;
    if (i < 0) { server.send(404, "text/plain", "personaje desconocido"); return; }
    FaceCtx ctx;
    ctx.expr = constrain(server.arg("e").toInt(), 0, E_COUNT - 1);
    ctx.t = server.hasArg("t") ? server.arg("t").toFloat() : 1.0f;
    ctx.clock = clockText();
    if (server.arg("tab") == "status") renderStatus(canvas, characterAt(i), battery, notifs, nNotifs, clockText());
    else if (server.arg("tab") == "net") renderNet();
    else if (server.arg("tab") == "setup") renderSetup();
    else renderFace(canvas, characterAt(i), ctx);
    sendBmp(server.arg("s").toInt() == 2 ? 2 : 1, server.hasArg("v"));
  });
  // Lo que se esta mostrando ahora mismo en la pantalla (cara, estado, QR...)
  server.on("/api/screen", HTTP_GET, []() { sendBmp(1, false); });
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/api/list", HTTP_GET, []() {
    String s = "[";
    char buf[200];
    for (int i = 0; i < characterCount(); i++) {
      const Character& c = characterAt(i);
      snprintf(buf, sizeof buf, "%s{\"id\":\"%s\",\"name\":\"%s\",\"style\":\"%s\",\"bg\":\"#%06X\",\"fg\":\"#%06X\"}",
               i ? "," : "", c.id, c.name, c.style, (unsigned)c.bg, (unsigned)c.fg);
      s += buf;
    }
    server.send(200, "application/json", s + "]");
  });
  server.on("/api/state", HTTP_GET, []() { server.send(200, "application/json", stateJson()); });
  server.on("/api/set", HTTP_ANY, []() {
    if (server.hasArg("c")) {
      int i = characterIndex(server.arg("c").c_str());
      if (i >= 0) { curChar = i; prefs.putString("char", characterAt(i).id); }
    }
    if (server.hasArg("e")) {
      int e = server.arg("e").toInt();
      forcedExpr = (e >= 0 && e < E_COUNT) ? e : -1;
      if (screen != SC_SETUP) setScreen(SC_FACE);
    }
    server.send(200, "application/json", stateJson());
  });
  // Diagnostico del pin de la pila: lectura con varias atenuaciones y estado digital con pull-down / pull-up
  server.on("/api/adc", HTTP_GET, []() {
    if (PIN_BAT_ADC < 0) { server.send(200, "application/json", "{}"); return; }
    const adc_attenuation_t at[4] = {ADC_0db, ADC_2_5db, ADC_6db, ADC_11db};
    const char* nm[4] = {"mv_0dB", "mv_2_5dB", "mv_6dB", "mv_11dB"};
    String s = "{";
    for (int i = 0; i < 4; i++) {
      analogSetPinAttenuation(PIN_BAT_ADC, at[i]);
      delay(5);
      s += String("\"") + nm[i] + "\":" + analogReadMilliVolts(PIN_BAT_ADC) + ",";
    }
    analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);
    pinMode(PIN_BAT_ADC, INPUT_PULLDOWN);
    delay(20);
    int dpd = digitalRead(PIN_BAT_ADC);
    pinMode(PIN_BAT_ADC, INPUT_PULLUP);
    delay(20);
    int dpu = digitalRead(PIN_BAT_ADC);
    pinMode(PIN_BAT_ADC, INPUT);
    s += "\"digital_con_pulldown\":" + String(dpd) + ",\"digital_con_pullup\":" + String(dpu) + "}";
    server.send(200, "application/json", s);
  });
  server.on("/api/battery", HTTP_ANY, []() {
    if (batterySim) battery = constrain(server.arg("v").toInt(), 0, 100);
    server.send(200, "application/json", stateJson());
  });
  server.on("/api/notify", HTTP_ANY, []() {
    addNotif(server.arg("from"), server.arg("text"));
    server.send(200, "application/json", stateJson());
  });
  server.on("/api/clear", HTTP_ANY, []() {
    nNotifs = 0;
    notifUntil = 0;
    server.send(200, "application/json", stateJson());
  });
  server.on("/api/scan", HTTP_GET, []() {
    scanning = false;  // evita chocar con la busqueda en segundo plano
    WiFi.scanDelete();
    int n = WiFi.scanNetworks();
    lastScanAt = millis();
    String s = "[";
    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i).isEmpty()) continue;
      s += String(s.length() > 1 ? "," : "") + "\"" + jsonEsc(WiFi.SSID(i)) + "\"";
    }
    WiFi.scanDelete();
    server.send(200, "application/json", s + "]");
  });
  server.on("/api/wifi", HTTP_ANY, []() {
    String ssid = server.arg("ssid");
    if (ssid.isEmpty()) { server.send(400, "application/json", "{\"error\":\"falta ssid\"}"); return; }
    int known = findNet(ssid);
    String pass = (!server.hasArg("pass") && known >= 0) ? nets[known].pass : server.arg("pass");
    rememberNet(ssid, pass);
    everConnected = false;
    triedMask = 0;
    server.send(200, "application/json", stateJson());
    delay(100);
    tryingIdx = 0;
    startWifi(ssid, pass);  // si no conecta, sigue con las demas redes guardadas y luego abre el modo configuracion
  });
  server.on("/api/forget", HTTP_ANY, []() {
    forgetNet(server.arg("ssid"));
    server.send(200, "application/json", stateJson());
  });
  server.on("/api/time", HTTP_ANY, []() {
    long long t = atoll(server.arg("t").c_str());
    if (t > 1600000000LL) {
      struct timeval tv = {(time_t)t, 0};
      settimeofday(&tv, nullptr);
      timeFromNtp = false;
    }
    server.send(200, "application/json", stateJson());
  });
  server.onNotFound([]() {  // portal cautivo: cualquier URL lleva a la pagina de configuracion
    if (setupMode) {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain", "No encontrado");
    }
  });
  server.begin();
}

// ------------------------------------------------------------ boton capacitivo (TTP223)
static void pollButton() {
  static bool last = false, longDone = false;
  static uint32_t pressedAt = 0, lastChange = 0;
  bool now = digitalRead(PIN_BTN) == HIGH;
  if (now != last && millis() - lastChange > 60) {
    lastChange = millis();
    last = now;
    if (now) {
      pressedAt = millis();
      longDone = false;
    } else if (!longDone) {  // toque corto
      if (screen == SC_FACE) setScreen(SC_STATUS);
      else if (screen == SC_STATUS) setScreen(SC_NET);
      else setScreen(SC_FACE);
    }
  }
  if (last && !longDone && millis() - pressedAt >= LONG_PRESS_MS) {
    longDone = true;
    startSetup(true);
  }
}

// ------------------------------------------------------------ pantallas de configuracion
static void drawCentered(const char* s, int y, uint32_t col, const lgfx::IFont* f) {
  canvas.setFont(f);
  canvas.setTextColor(col);
  canvas.setTextDatum(textdatum_t::top_center);
  canvas.drawString(s, 120, y);
}

static void renderSetup() {
  canvas.fillScreen(0xFFFFFF);
  char qr[80];
  snprintf(qr, sizeof qr, "WIFI:T:WPA;S:%s;P:%s;;", AP_SSID, AP_PASS);
  canvas.qrcode(qr, 40, 6, 160, 3);
  drawCentered("Escanea para conectarte", 172, 0x000000, &fonts::DejaVu9);
  drawCentered(AP_SSID, 188, 0x000000, &fonts::FreeSansBold9pt7b);
  drawCentered("y abre 192.168.4.1", 212, 0x444444, &fonts::DejaVu9);
  drawTabDots(canvas, 0x000000, 0xFFFFFF, 2);
}

static void renderNet() {
  const uint32_t BGC = 0x0B1020, MUTED = 0x8F9AB8;
  if (WiFi.status() != WL_CONNECTED && setupMode) { renderSetup(); return; }
  canvas.fillScreen(BGC);
  canvas.setFont(&fonts::DejaVu9);
  canvas.setTextColor(MUTED);
  canvas.setTextDatum(textdatum_t::top_left);
  canvas.drawString("RED", 14, 14);
  canvas.setTextDatum(textdatum_t::top_right);
  if (WiFi.status() == WL_CONNECTED) {
    canvas.setTextColor((uint32_t)0x2FB36B);
    canvas.drawString("CONECTADO", 226, 14);
    String ssid = WiFi.SSID();
    if (ssid.length() > 24) ssid = ssid.substring(0, 23) + "...";
    String ip = WiFi.localIP().toString();
    drawCentered(ssid.c_str(), 34, 0xFFFFFF, &fonts::DejaVu12);
    drawCentered(ip.c_str(), 58, 0x7DF9FF, &fonts::FreeSansBold12pt7b);
    drawCentered("o  llavero.local", 88, MUTED, &fonts::DejaVu9);
    canvas.fillRoundRect(60, 108, 120, 120, 6, 0xFFFFFF);
    char url[40];
    snprintf(url, sizeof url, "http://%s/", ip.c_str());
    canvas.qrcode(url, 66, 114, 108, 3);
  } else {
    canvas.setTextColor((uint32_t)0xE5484D);
    canvas.drawString("SIN CONEXION", 226, 14);
    drawCentered("Sin WiFi", 58, 0xFFFFFF, &fonts::FreeSansBold12pt7b);
    char line[40];
    snprintf(line, sizeof line, "Redes guardadas: %d", nNets);
    drawCentered(line, 100, MUTED, &fonts::DejaVu9);
    drawCentered("Busca la red automaticamente.", 130, MUTED, &fonts::DejaVu9);
    drawCentered("Para configurarla, manten", 162, 0xFFFFFF, &fonts::DejaVu9);
    drawCentered("el boton 4 segundos.", 178, 0xFFFFFF, &fonts::DejaVu9);
  }
  drawTabDots(canvas, 0xFFFFFF, BGC, 2);
}

// ------------------------------------------------------------ arranque
void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN, INPUT_PULLDOWN);
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, LOW);
  if (PIN_BAT_ADC >= 0) analogReadResolution(12);
  applyTz();

  initCharacters();
  prefs.begin("llavero", false);
  int saved = characterIndex(prefs.getString("char", "1a").c_str());
  curChar = saved >= 0 ? saved : 0;
  loadNets();

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  digitalWrite(PIN_TFT_BL, HIGH);

  canvas.setColorDepth(16);
  if (!canvas.createSprite(240, 240)) Serial.println("ERROR: sin memoria para el sprite 240x240");

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.setAutoReconnect(false);  // wifiLoop() elige la mejor red guardada que se vea
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();
  setupWeb();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  pollButton();
  wifiLoop();
  updateBattery();
  updateAutoExpr();

  // vuelve solo a la cara
  if ((screen == SC_STATUS || screen == SC_NET) && millis() - screenSince > screenTimeout) setScreen(SC_FACE);

  static uint32_t lastFrame = 0;
  if (millis() - lastFrame < 33) return;
  lastFrame = millis();

  const Character& ch = characterAt(curChar);
  switch (screen) {
    case SC_STATUS:
      renderStatus(canvas, ch, battery, notifs, nNotifs, clockText());
      break;
    case SC_SETUP:
      renderSetup();
      break;
    case SC_NET:
      renderNet();
      break;
    default: {
      FaceCtx ctx;
      ctx.expr = currentExpr();
      ctx.t = millis() / 1000.0f;
      ctx.clock = clockText();
      ctx.notifCount = nNotifs;
      for (int i = 0; i < nNotifs && i < 3; i++) ctx.notifCol[i] = notifs[i].color;
      renderFace(canvas, ch, ctx);
    }
  }
  canvas.pushSprite(0, 0);
}
