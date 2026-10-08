#include "faces.h"
#include <math.h>

// Port a C++ de las funciones minimal(), kawaii(), robot(), pixel(), fx() y hud()
// de UI/design_handoff_llavero_amigo/Llavero Amigo.dc.html. Coordenadas en pixeles (0-240).

static LGFX_Sprite* S;
static uint32_t BG;
static const FaceCtx* X;

// ---------------------------------------------------------------- helpers
static inline int R(float v) { return (int)lroundf(v); }

static uint32_t mix(uint32_t a, uint32_t b, float t) {  // t=1 -> a, t=0 -> b
  if (t >= 1) return a;
  if (t <= 0) return b;
  auto ch = [&](int sh) { return (uint32_t)lroundf(((a >> sh) & 0xFF) * t + ((b >> sh) & 0xFF) * (1 - t)); };
  return (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

static float blinkf(float t, float per, float off = 0) {
  float p = fmodf(fmodf(t + off, per) + per, per);
  return p < 0.18f ? fmaxf(0.06f, fabsf(p - 0.09f) / 0.09f) : 1.0f;
}

static void rrect(float x, float y, float w, float h, float r, uint32_t c) {
  if (w < 1 || h < 1) return;
  r = fminf(r, fminf(w, h) / 2);
  S->fillRoundRect(R(x), R(y), R(w), R(h), R(r), c);
}

static void rrectStroke(float x, float y, float w, float h, float r, uint32_t fill, uint32_t stroke, float sw) {
  rrect(x - sw / 2, y - sw / 2, w + sw, h + sw, r + sw / 2, stroke);
  rrect(x + sw / 2, y + sw / 2, w - sw, h - sw, r - sw / 2, fill);
}

static void ell(float cx, float cy, float rx, float ry, uint32_t c) {
  if (rx < 0.5f || ry < 0.5f) return;
  S->fillEllipse(R(cx), R(cy), R(rx), R(ry), c);
}

static void circ(float cx, float cy, float r, uint32_t c) {
  if (r < 0.5f) return;
  S->fillCircle(R(cx), R(cy), R(r), c);
}

static void ring(float cx, float cy, float r, float w, uint32_t c) {
  S->fillArc(R(cx), R(cy), R(r - w / 2), R(r + w / 2), 0, 360, c);
}

// poligono cualquiera visible desde (fx,fy), triangulado en abanico
static void fan(const float* p, int n, float fx, float fy, uint32_t c) {
  for (int i = 0; i < n; i++) {
    int j = (i + 1) % n;
    S->fillTriangle(R(fx), R(fy), R(p[i * 2]), R(p[i * 2 + 1]), R(p[j * 2]), R(p[j * 2 + 1]), c);
  }
}

static void rotEll(float cx, float cy, float rx, float ry, float deg, uint32_t c) {
  const int n = 28;
  float p[n * 2], a = deg * PI / 180.0f, ca = cosf(a), sa = sinf(a);
  for (int i = 0; i < n; i++) {
    float th = i * 2 * PI / n, x = rx * cosf(th), y = ry * sinf(th);
    p[i * 2] = cx + x * ca - y * sa;
    p[i * 2 + 1] = cy + x * sa + y * ca;
  }
  fan(p, n, cx, cy, c);
}

// curva cuadratica con trazo redondeado
static void qcurve(float x0, float y0, float x1, float y1, float x2, float y2, float w, uint32_t c) {
  const int n = 14;
  for (int i = 0; i <= n; i++) {
    float u = (float)i / n, a = (1 - u) * (1 - u), b = 2 * u * (1 - u), d = u * u;
    circ(a * x0 + b * x1 + d * x2, a * y0 + b * y1 + d * y2, w / 2, c);
  }
}

static void heart(float cx, float cy, float s, uint32_t c) {
  float k = s / 2;
  float seg[4][8] = {
      {cx, cy + k * 0.95f, cx - k * 0.2f, cy + k * 0.7f, cx - k, cy + k * 0.3f, cx - k, cy - k * 0.25f},
      {cx - k, cy - k * 0.25f, cx - k, cy - k * 0.8f, cx - k * 0.2f, cy - k * 0.95f, cx, cy - k * 0.45f},
      {cx, cy - k * 0.45f, cx + k * 0.2f, cy - k * 0.95f, cx + k, cy - k * 0.8f, cx + k, cy - k * 0.25f},
      {cx + k, cy - k * 0.25f, cx + k, cy + k * 0.3f, cx + k * 0.2f, cy + k * 0.7f, cx, cy + k * 0.95f}};
  const int per = 8;
  float p[4 * per * 2];
  int n = 0;
  for (auto& s4 : seg)
    for (int i = 0; i < per; i++) {
      float u = (float)i / per, v = 1 - u;
      float b0 = v * v * v, b1 = 3 * v * v * u, b2 = 3 * v * u * u, b3 = u * u * u;
      p[n * 2] = b0 * s4[0] + b1 * s4[2] + b2 * s4[4] + b3 * s4[6];
      p[n * 2 + 1] = b0 * s4[1] + b1 * s4[3] + b2 * s4[5] + b3 * s4[7];
      n++;
    }
  fan(p, n, cx, cy, c);
}

// resplandor aproximado: capas mas grandes y translucidas bajo la forma
static void glowRect(float x, float y, float w, float h, float r, uint32_t c) {
  const float ex[3] = {8, 5, 2}, al[3] = {0.10f, 0.18f, 0.30f};
  for (int i = 0; i < 3; i++) rrect(x - ex[i], y - ex[i], w + 2 * ex[i], h + 2 * ex[i], r + ex[i], mix(c, BG, al[i]));
}
static void glowHeart(float cx, float cy, float s, uint32_t c) {
  const float ex[3] = {12, 8, 4}, al[3] = {0.12f, 0.2f, 0.32f};
  for (int i = 0; i < 3; i++) heart(cx, cy, s + ex[i], mix(c, BG, al[i]));
}

// ---------------------------------------------------------------- efectos y HUD
static void fx(const Character& c, int expr, float t) {
  if (expr == E_AMOR)
    for (int i = 0; i < 3; i++) {
      float p = fmodf(t * 0.45f + i / 3.0f, 1.0f);
      heart(36 + i * 84 + sinf(t * 2 + i) * 10, 214 - p * 180, 16 + (i % 2) * 6, mix(c.love, BG, (1 - p) * 0.9f));
    }
  if (expr == E_BATERIA) {
    bool on = sinf(t * 5) > -0.3f;
    uint32_t fg = on ? c.fg : mix(c.fg, BG, 0.3f), red = on ? 0xFF3B3B : mix(0xFF3B3B, BG, 0.3f);
    rrectStroke(101, 14, 34, 18, 4, BG, fg, 2.5f);
    rrect(135.5f, 19.5f, 3, 7, 1, fg);
    rrect(104.5f, 17.5f, 6, 11, 1.5f, red);
  }
  if (expr == E_NOTIF) {
    float ph = fmodf(t * 1.4f, 1.0f), pop = 1 + 0.12f * fabsf(sinf(t * 5));
    ring(204, 38, 18 + ph * 20, 3, mix(c.accent, BG, 1 - ph));
    circ(204, 38, 18 * pop, c.accent);
    S->setFont(&fonts::FreeSansBold12pt7b);
    S->setTextColor(c.bg);
    S->setTextDatum(textdatum_t::middle_center);
    char num[4];
    if (X->notifCount > 0) snprintf(num, sizeof num, "%d", X->notifCount);
    else strcpy(num, "!");
    S->drawString(num, 204, 38);
  }
}

static void hud(const Character& c, int expr, const char* clock) {
  if (clock) {
    S->setFont(c.type == T_PIXEL ? (const lgfx::IFont*)&fonts::FreeSansBold9pt7b : (const lgfx::IFont*)&fonts::FreeSansBold12pt7b);
    S->setTextColor(mix(c.fg, BG, 0.9f));
    S->setTextDatum(textdatum_t::baseline_left);
    S->drawString(clock, 14, 26);
  }
  if (expr != E_NOTIF && expr != E_BATERIA)
    for (int i = 0; i < X->notifCount && i < 3; i++) {
      if (c.type == T_PIXEL) S->fillRect(196 + i * 12, 15, 8, 8, X->notifCol[i]);
      else circ(202 + i * 12, 20, 4.5f, X->notifCol[i]);
    }
}

void drawTabDots(LGFX_Sprite& s, uint32_t color, uint32_t bg, int active) {
  S = &s;
  for (int i = 0; i < 3; i++) circ(110 + i * 10, 234, 2.5f, mix(color, bg, i == active ? 1.0f : 0.3f));
}

// ---------------------------------------------------------------- minimal
static void minimalFace(const Character& c, int expr, float t) {
  float bob = sinf(t * 2) * 4, lx = 0, open = blinkf(t, 3.4f), sc = 1, ey = 120;
  if (expr == E_PARPADEO) { open = blinkf(t, 1.1f); lx = sinf(t * 1.3f) * 14; }
  if (expr == E_NOTIF) { bob = -fabsf(sinf(t * 5)) * 12; sc = 1.15f; open = blinkf(t, 4); }
  if (expr == E_BATERIA) { bob = sinf(t * 0.8f) * 2; ey = 134; open = blinkf(t, 5); }
  if (expr == E_AMOR) bob = sinf(t * 3) * 5;
  for (int i = 0; i < 2; i++) {
    float x = (i ? 160 : 80) + lx, y = ey + bob;
    if (expr == E_AMOR) {
      float s = 64 * (1 + 0.1f * sinf(t * 7));
      if (c.glow) glowHeart(x, y, s, c.love);
      heart(x, y, s, c.love);
      continue;
    }
    float ww = c.eyeW * sc, hh = (expr == E_BATERIA ? c.eyeH * 0.5f : c.eyeH * sc) * open;
    float r = fminf((float)c.eyeR, hh / 2);
    if (c.glow) glowRect(x - ww / 2, y - hh / 2, ww, hh, r, c.eye);
    rrect(x - ww / 2, y - hh / 2, ww, hh, r, c.eye);
    if (expr == E_FELIZ && open > 0.5f) ell(x, y + hh / 2 + 10, ww * 0.85f, hh * 0.45f, c.bg);
    if (expr == E_BATERIA) {
      float L = x - ww / 2 - 4, Rr = x + ww / 2 + 4, T = y - hh / 2 - 4, yy = y + hh * 0.15f;
      if (i == 0) S->fillTriangle(R(L), R(T), R(Rr), R(T), R(L), R(yy), c.bg);
      else S->fillTriangle(R(L), R(T), R(Rr), R(T), R(Rr), R(yy), c.bg);
    }
  }
}

// ---------------------------------------------------------------- kawaii
static float kBob, kRot;
static void T(float& x, float& y) {  // rotate(rot 120 150) + translate(0 bob)
  float a = kRot * PI / 180.0f, dx = x - 120, dy = y - 150;
  x = 120 + dx * cosf(a) - dy * sinf(a);
  y = 150 + dx * sinf(a) + dy * cosf(a) + kBob;
}
static void tEll(float cx, float cy, float rx, float ry, uint32_t c) { T(cx, cy); ell(cx, cy, rx, ry, c); }
static void tCirc(float cx, float cy, float r, uint32_t c) { T(cx, cy); circ(cx, cy, r, c); }
static void tCurve(float x0, float y0, float x1, float y1, float x2, float y2, float w, uint32_t c) {
  T(x0, y0); T(x1, y1); T(x2, y2);
  qcurve(x0, y0, x1, y1, x2, y2, w, c);
}
static void tTri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c) {
  T(x0, y0); T(x1, y1); T(x2, y2);
  S->fillTriangle(R(x0), R(y0), R(x1), R(y1), R(x2), R(y2), c);
}

static void kawaiiFace(const Character& c, int expr, float t) {
  kBob = sinf(t * 2.2f) * 4;
  kRot = 0;
  if (expr == E_NOTIF) kBob = -fabsf(sinf(t * 5)) * 14;
  if (expr == E_BATERIA) { kBob = 5; kRot = sinf(t * 1.2f) * 4; }

  switch (c.shape) {
    case S_MOCHI:
      tEll(120, 232, 90, 10, mix(0x000000, BG, 0.08f));
      tEll(120, 152, 104, 84, c.face);
      break;
    case S_NUBE: {
      const float n[6][3] = {{64, 150, 48}, {118, 118, 62}, {176, 148, 50}, {94, 180, 46}, {150, 182, 46}, {120, 160, 64}};
      for (auto& p : n) tCirc(p[0], p[1], p[2], c.face);
      break;
    }
    case S_PERRO: {
      tEll(120, 148, 94, 82, c.face);
      tEll(120, c.eyeY + 32, 42, 28, c.muzzle);
      float x = 34, y = 132; T(x, y); rotEll(x, y, 25, 54, 14, c.ear);
      x = 206; y = 132; T(x, y); rotEll(x, y, 25, 54, -14, c.ear);
      break;
    }
    default:  // gato
      tTri(34, 108, 54, 34, 108, 74, c.face);
      tTri(206, 108, 186, 34, 132, 74, c.face);
      tTri(50, 92, 60, 54, 88, 76, 0xFFC9B0);
      tTri(190, 92, 180, 54, 152, 76, 0xFFC9B0);
      tEll(120, 150, 102, 82, c.face);
  }

  float ey = c.eyeY, my = ey + 30 + c.mouthDy;
  uint32_t under = c.shape == S_NUBE || c.shape == S_MOCHI ? c.face : c.shape == S_PERRO ? c.muzzle : c.face;

  if (c.nose == N_CAT) tTri(114, ey + 13, 126, ey + 13, 120, ey + 22, c.noseCol);
  if (c.nose == N_DOG) {
    tEll(120, ey + 20, 11, 8, c.noseCol);
    tEll(116, ey + 17, 3.5f, 2, mix(0xFFFFFF, c.noseCol, 0.7f));
  }
  for (int i = 0; i < 2; i++)
    tEll(i ? 178 : 62, ey + 22, 15, 8, mix(c.blush, c.face, expr == E_AMOR ? 1.0f : 0.7f));
  (void)under;

  for (int i = 0; i < 2; i++) {
    float x = i ? 152 : 88;
    if (expr == E_AMOR) {
      float cx = x, cy = ey; T(cx, cy);
      heart(cx, cy, 36 * (1 + 0.1f * sinf(t * 7)), c.love);
      continue;
    }
    if (expr == E_BATERIA) { tCurve(x - 13, ey, x, ey + 6, x + 13, ey, 4.5f, c.eye); continue; }
    float o = expr == E_PARPADEO ? blinkf(t, 1.3f) : blinkf(t, 3.6f, i * 0.02f);
    float rx = expr == E_NOTIF ? 15 : 12.5f, ry = (expr == E_NOTIF ? 19 : 15) * o;
    if (o < 0.35f) { tCurve(x - 12, ey, x, ey + 7, x + 12, ey, 4.5f, c.eye); continue; }
    tEll(x, ey, rx, ry, c.eye);
    tCirc(x - 4, ey - ry * 0.4f, fmaxf(1.0f, rx * 0.36f), 0xFFFFFF);
    tCirc(x + 4, ey + ry * 0.35f, fmaxf(1.0f, rx * 0.18f), 0xFFFFFF);
  }

  const float w = 3.5f;
  if (expr == E_FELIZ) {
    const int n = 12;
    float p[(n + 1) * 2];
    for (int i = 0; i <= n; i++) {
      float u = (float)i / n, a = (1 - u) * (1 - u), b = 2 * u * (1 - u), d = u * u;
      float x = a * 106 + b * 120 + d * 134, y = a * my + b * (my + 24) + d * my;
      T(x, y);
      p[i * 2] = x; p[i * 2 + 1] = y;
    }
    float fx0 = 120, fy0 = my + 3; T(fx0, fy0);
    fan(p, n + 1, fx0, fy0, c.mouth);
  }
  if (expr == E_PARPADEO) {
    tCurve(108, my, 114, my + 8, 120, my, w, c.mouth);
    tCurve(120, my, 126, my + 8, 132, my, w, c.mouth);
  }
  if (expr == E_AMOR) tCurve(110, my, 120, my + 11, 130, my, w, c.mouth);
  if (expr == E_BATERIA) {
    tCurve(108, my + 4, 112, my, 116, my + 4, w, c.mouth);
    tCurve(116, my + 4, 120, my + 8, 124, my + 4, w, c.mouth);
    tCurve(124, my + 4, 128, my, 132, my + 4, w, c.mouth);
    float dp = fmodf(t * 0.6f, 1.0f), y0 = ey - 34 + dp * 10;
    const int n = 8;
    float p[(2 * n) * 2];
    int m = 0;
    for (int i = 0; i < n; i++) {  // 196,y0 -> ctrl 188,y0+12 -> 196,y0+16
      float u = (float)i / n, a = (1 - u) * (1 - u), b = 2 * u * (1 - u), d = u * u;
      float x = a * 196 + b * 188 + d * 196, y = a * y0 + b * (y0 + 12) + d * (y0 + 16);
      T(x, y); p[m * 2] = x; p[m * 2 + 1] = y; m++;
    }
    for (int i = 0; i < n; i++) {  // 196,y0+16 -> ctrl 204,y0+12 -> 196,y0
      float u = (float)i / n, a = (1 - u) * (1 - u), b = 2 * u * (1 - u), d = u * u;
      float x = a * 196 + b * 204 + d * 196, y = a * (y0 + 16) + b * (y0 + 12) + d * y0;
      T(x, y); p[m * 2] = x; p[m * 2 + 1] = y; m++;
    }
    float cx = 196, cy = y0 + 8; T(cx, cy);
    fan(p, m, cx, cy, mix(0x7CC7FF, BG, 1 - dp * 0.6f));
  }
  if (expr == E_NOTIF) tEll(120, my + 6, 7, 9, c.mouth);
}

// ---------------------------------------------------------------- robot
static void robotFace(const Character& c, int expr, float t) {
  bool A = c.shape == 1;
  float bob = sinf(t * 2) * 3;
  if (expr == E_NOTIF) bob = -fabsf(sinf(t * 6)) * 8;

  if (A) {
    float per = expr == E_NOTIF ? 0.2f : expr == E_BATERIA ? 2.0f : 0.9f;
    bool on = ((int)floorf(t / per)) % 2 == 0;
    S->fillRect(118, 30, 5, 32, 0x6C7480);
    circ(120, 24, 9, on ? (expr == E_BATERIA ? 0xFF3B3B : c.led) : 0x6C7480);
    rrectStroke(16, 58, 208, 168, 30, c.plate, c.plateStroke, 3);
    const int sx[4] = {34, 206, 34, 206}, sy[4] = {76, 76, 208, 208};
    for (int i = 0; i < 4; i++) circ(sx[i], sy[i], 4.5f, c.plateStroke);
  } else {
    rrect(4, 90, 14, 44, 4, c.visorStroke);
    rrect(222, 90, 14, 44, 4, c.visorStroke);
    if (c.noMouth) rrectStroke(18, 40, 204, 160, 36, c.visor, c.visorStroke, 3);
    else rrectStroke(18, 50, 204, 124, 32, c.visor, c.visorStroke, 3);
    if (expr == E_PARPADEO) {
      float y = 56 + fmodf(t * 70, 110);
      S->fillRect(24, R(y), 192, 2, mix(c.eye, c.visor, 0.25f));
    }
  }

  float ey = (A ? 124 : 112) + bob + (c.noMouth ? 8 : 0);
  float lx = expr == E_PARPADEO ? sinf(t * 1.4f) * 10 : 0;
  for (int i = 0; i < 2; i++) {
    float cx = i ? 160 : 80, x = cx + lx;
    if (expr == E_AMOR) {
      float s = (A ? 46 : 50) * (1 + 0.1f * sinf(t * 7));
      if (c.glow) glowHeart(x, ey, s, c.love);
      heart(x, ey, s, c.love);
      continue;
    }
    float o = expr == E_PARPADEO ? blinkf(t, 1.1f) : blinkf(t, 3.8f);
    if (A) {
      float Rr = expr == E_NOTIF ? 30 : 26;
      ell(cx, ey, Rr, Rr * o, c.eyeDark);
      if (o < 0.5f) continue;
      if (expr == E_FELIZ) {
        qcurve(x - 14, ey + 6, x, ey - 16, x + 14, ey + 6, 6, c.led);
      } else if (expr == E_BATERIA) {
        circ(x, ey + 6, 9, mix(c.led, c.eyeDark, 0.5f));
        S->fillRect(R(cx - Rr - 2), R(ey - Rr - 2), R(Rr * 2 + 4), R(Rr + 2), c.plate);
        S->fillRect(R(cx - Rr), R(ey - 2), R(Rr * 2), 4, c.eyeDark);
      } else {
        circ(x, ey, expr == E_NOTIF ? 16 : 13, c.led);
        circ(x - 4, ey - 5, 4, mix(0xFFFFFF, c.led, 0.85f));
        if (expr == E_NOTIF) {
          float ph = fmodf(t * 2, 1.0f);
          ring(cx, ey, Rr + 2 + ph * 8, 3, mix(c.led, c.plate, 1 - ph));
        }
      }
    } else {
      float s = expr == E_NOTIF ? 54 : 44, hh = (expr == E_BATERIA ? 14 : s) * o;
      float flick = expr == E_BATERIA ? (sinf(t * 23) > 0.6f ? 0.35f : 0.7f) : 1.0f;
      float yy = ey - hh / 2 + (expr == E_BATERIA ? 10 : 0);
      uint32_t col = mix(c.eye, c.visor, flick);
      if (c.glow) glowRect(x - s / 2, yy, s, hh, fminf(9, hh / 2), col);
      rrect(x - s / 2, yy, s, hh, fminf(9, hh / 2), col);
      if (expr == E_FELIZ && o > 0.5f) ell(x, ey + hh / 2 + 8, s * 0.85f, hh * 0.42f, c.visor);
    }
  }

  if (c.noMouth) return;
  int n = A ? 7 : 11;
  float step = 12, x0 = 120 - (n - 1) * step / 2, y0 = (A ? 188 : 196) + bob;
  if (A) rrect(x0 - 12, y0 - 14, (n - 1) * step + 24, 30, 9, c.eyeDark);
  if (expr == E_NOTIF) {
    ring(120, y0, 9, 5, c.led);
    return;
  }
  uint32_t grillBg = A ? c.eyeDark : c.bg;
  for (int i = 0; i < n; i++) {
    float u = (i - (n - 1) / 2.0f) / ((n - 1) / 2.0f), y = y0;
    bool lit = true;
    uint32_t col = c.led;
    if (expr == E_FELIZ) y = y0 - 5 + (A ? 7 : 10) * (1 - u * u);
    if (expr == E_AMOR) { y = y0 - 3 + 6 * (1 - u * u); lit = fabsf(u) < 0.75f; }
    if (expr == E_PARPADEO) lit = fabsf(i - fmodf(t * 8, (float)(n + 3))) < 2;
    if (expr == E_BATERIA) { y = y0 + 2 * sinf(i + t * 3); lit = i < 2; col = 0xFF3B3B; }
    rrect(x0 + i * step - 4, y - 4, 8, 8, 2, lit ? col : mix(col, grillBg, 0.15f));
  }
}

// ---------------------------------------------------------------- pixel
struct BMp { uint8_t h; const char* r[6]; };
static const BMp BM_open = {3, {"w##", "###", "###"}};
static const BMp BM_happy = {2, {".#.", "#.#"}};
static const BMp BM_closed = {1, {"###"}};
static const BMp BM_heart = {4, {"##.##", "#####", ".###.", "..#.."}};
static const BMp BM_heartS = {3, {"#.#", "###", ".#."}};
static const BMp BM_sleepL = {2, {"#..", "###"}};
static const BMp BM_sleepR = {2, {"..#", "###"}};
static const BMp BM_wide = {4, {"w###", "####", "####", "####"}};
static const BMp BM_smile = {2, {"#...#", ".###."}};
static const BMp BM_small = {2, {"#.#", ".#."}};
static const BMp BM_wavy = {2, {".#.#.", "#.#.#"}};
static const BMp BM_o = {3, {".#.", "#.#", ".#."}};
static const BMp BM_bat = {3, {"######.", "#r...##", "######."}};
static const BMp BM_bang = {6, {".###.", "##.##", "##.##", "#####", "##.##", ".###."}};
static const BMp BM_ph = {3, {"#.#", "###", ".#."}};
static const BMp BM4_feliz = {2, {"#..#", ".##."}};
static const BMp BM4_parp = {1, {".##."}};
static const BMp BM4_bat = {2, {"#.#.", ".#.#"}};
static const BMp BM4_notif = {3, {".##.", "#..#", ".##."}};

static int32_t G[20][20];
static int GN;

static void gset(int x, int y, int32_t col) {
  if (x >= 0 && x < GN && y >= 0 && y < GN) G[y][x] = col;
}
static void stamp(const BMp& bm, int x, int y, int32_t cHash, int32_t cW, int32_t cR, bool under = false) {
  for (int j = 0; j < bm.h; j++)
    for (int i = 0; bm.r[j][i]; i++) {
      char ch = bm.r[j][i];
      if (ch == '.') continue;
      int32_t col = ch == '#' ? cHash : ch == 'w' ? cW : cR;
      if (col < 0) continue;
      int gx = x + i, gy = y + j;
      if (under && gx >= 0 && gx < GN && gy >= 0 && gy < GN && G[gy][gx] >= 0) continue;
      gset(gx, gy, col);
    }
}

static int32_t bodyAt(const Character& c, int x, int yy, int fr) {
  auto sq = [](float v) { return v * v; };
  switch (c.body) {
    case B_BLOB:
      return (sq((x - 9.5f) / 8.6f) + sq((yy - 12.5f) / 6.6f) <= 1 || (yy >= 15 && yy <= 18 && x >= 2 && x <= 17)) ? c.bodyCol : -1;
    case B_GHOST:
      return ((sq(x - 9.5f) + sq(yy - 9.0f) <= 49 && yy <= 9) || (yy > 9 && yy <= 16 && x >= 3 && x <= 16) ||
              (yy == 17 && x >= 3 && x <= 16 && (x + fr) % 4 < 2)) ? c.bodyCol : -1;
    case B_CAT: {
      if (sq((x - 9.5f) / 8.5f) + sq((yy - 12.0f) / 6.5f) <= 1) return c.bodyCol;
      int xm = x < 10 ? x : 19 - x;
      if (yy >= 3 && yy <= 6 && xm >= (yy == 6 ? 2 : 3) && xm <= yy) return (xm == 4 && yy == 5) ? c.inner : c.bodyCol;
      return -1;
    }
    case B_DOG:
      if (sq((x - 2.5f) / 2) + sq((yy - 10.0f) / 4.5f) <= 1 || sq((x - 16.5f) / 2) + sq((yy - 10.0f) / 4.5f) <= 1) return c.ear;
      if (sq((x - 9.5f) / 3.6f) + sq((yy - 14.5f) / 2.4f) <= 1) return c.muzzle;
      if (sq((x - 9.5f) / 7.5f) + sq((yy - 11.0f) / 7) <= 1) return c.bodyCol;
      return -1;
    case B_SKULL:  // craneo + mandibula
      if (sq((x - 9.5f) / 7.7f) + sq((yy - 8.0f) / 7.0f) <= 1) return c.bodyCol;
      return (yy >= 14 && yy <= 18 && x >= 5 && x <= 14) ? c.bodyCol : -1;
    case B_TREE: {  // tres pisos + tronco + estrella
      float dx = fabsf(x - 9.5f);
      if (yy >= 15 && yy <= 17 && x >= 8 && x <= 11) return c.ear;
      if ((yy == 0 && x >= 9 && x <= 10) || (yy == 1 && x >= 8 && x <= 11)) return c.inner;
      if (yy >= 2 && yy <= 6 && dx <= (yy - 2) + 0.5f) return c.bodyCol;
      if (yy >= 6 && yy <= 10 && dx <= (yy - 6) + 2.5f) return c.bodyCol;
      if (yy >= 10 && yy <= 14 && dx <= (yy - 10) + 3.5f) return c.bodyCol;
      return -1;
    }
  }
  return -1;
}

static void pixelFace(const Character& c, int expr, float t) {
  GN = c.N;
  const int N = c.N, P = 240 / N;
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++) G[y][x] = -1;

  int fr = (int)floorf(t * 4);
  int dy = c.body == B_GHOST ? (int)lroundf(sinf(t * 2)) : ((int)floorf(t * 2)) % 2;
  if (expr == E_NOTIF) dy = fr % 2 ? -1 : 0;
  if (expr == E_BATERIA) dy = 1;

  if (c.body) {
    static int32_t B[20][20];
    for (int y = 0; y < N; y++)
      for (int x = 0; x < N; x++) B[y][x] = bodyAt(c, x, y - dy, fr);
    auto isB = [&](int x, int y) { return x >= 0 && x < N && y >= 0 && y < N && B[y][x] >= 0; };
    for (int y = 0; y < N; y++)
      for (int x = 0; x < N; x++)
        if (B[y][x] >= 0) {
          const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
          for (auto& q : d)
            if (!isB(x + q[0], y + q[1])) gset(x + q[0], y + q[1], c.line);
        }
    for (int y = 0; y < N; y++)
      for (int x = 0; x < N; x++)
        if (B[y][x] >= 0) gset(x, y, B[y][x]);
    if (c.body == B_BLOB) { gset(4, 9 + dy, c.hi); gset(5, 8 + dy, c.hi); gset(4, 10 + dy, c.hi); }
    if (c.body == B_GHOST) { gset(5, 4 + dy, c.hi); gset(4, 5 + dy, 0xC9D0EA); }
    if (c.body == B_SKULL) { gset(7, 17 + dy, c.line); gset(10, 17 + dy, c.line); gset(13, 17 + dy, c.line); gset(13, 3 + dy, c.line); gset(14, 4 + dy, c.line); }
    if (c.body == B_TREE) {  // luces que parpadean
      static const int8_t orn[6][2] = {{7, 5}, {12, 6}, {4, 12}, {15, 13}, {6, 14}, {13, 14}};
      static const uint32_t oc[3] = {0xE5484D, 0xF5D547, 0x3D8BFF};
      for (int i = 0; i < 6; i++) gset(orn[i][0], orn[i][1] + dy, oc[(i + fr / 2) % 3]);
    }
  }

  if (expr == E_AMOR && N >= 16)
    for (int i = 0; i < 3; i++) {
      float p = fmodf(t * 0.5f + i / 3.0f, 1.0f);
      int xs[3] = {1, N - 4, N / 2 - 1};
      stamp(BM_ph, xs[i], (int)floorf(N - 3 - p * (N - 7)), c.love, -1, -1, true);
    }

  if (c.flat) {
    static const int8_t cat[6][2] = {{1, 0}, {1, 1}, {2, 1}, {1, 2}, {2, 2}, {3, 2}};
    static const int8_t dog[10][2] = {{1, 2}, {0, 3}, {1, 3}, {0, 4}, {1, 4}, {0, 5}, {1, 5}, {0, 6}, {1, 6}, {1, 7}};
    int n = c.flat == F_CAT ? 6 : 10;
    for (int i = 0; i < n; i++) {
      int x = c.flat == F_CAT ? cat[i][0] : dog[i][0], y = c.flat == F_CAT ? cat[i][1] : dog[i][1];
      gset(x, y + dy, c.mid);
      gset(N - 1 - x, y + dy, c.mid);
    }
  }

  static const int shifts[4] = {0, -1, 0, 1};
  int shift = expr == E_PARPADEO ? shifts[((int)floorf(t)) % 4] : 0;
  for (int i = 0; i < 2; i++) {
    int x = c.eyes[i][0] + shift, y = c.eyes[i][1] + dy;
    if (expr == E_FELIZ) {
      if (blinkf(t, 3.5f) < 0.5f) stamp(BM_closed, x, y + 2, c.dark, c.hi, c.love);
      else if (((int)floorf(t / 2)) % 3 == 2) stamp(BM_happy, x, y + 1, c.dark, c.hi, c.love);
      else stamp(BM_open, x, y, c.dark, c.hi, c.love);
    }
    if (expr == E_PARPADEO) {
      if (blinkf(t, 1.2f) < 0.6f) stamp(BM_closed, x, y + 2, c.dark, c.hi, c.love);
      else stamp(BM_open, x, y, c.dark, c.hi, c.love);
    }
    if (expr == E_AMOR) {
      if ((fr % 2) && N >= 16) stamp(BM_heart, x - 1, y, c.love, -1, -1);
      else stamp(BM_heartS, x, y, c.love, -1, -1);
    }
    if (expr == E_BATERIA) stamp(i ? BM_sleepR : BM_sleepL, x, y + 1, c.dark, c.hi, c.love);
    if (expr == E_NOTIF) stamp(BM_wide, x - (i ? 0 : 1), y - 1, c.dark, c.hi, c.love);
  }

  if (c.blush != NOCOL && expr != E_BATERIA) {
    int by = c.eyes[0][1] + 3 + dy;
    gset(c.eyes[0][0] - 1, by, c.blush); gset(c.eyes[0][0], by, c.blush);
    gset(c.eyes[1][0] + 2, by, c.blush); gset(c.eyes[1][0] + 3, by, c.blush);
  }
  if (c.hasNoseCells)
    for (int i = 0; i < 2; i++) gset(c.noseCells[i][0], c.noseCells[i][1] + dy, c.noseCol);

  const BMp* mb;
  if (c.flat) mb = expr == E_FELIZ || expr == E_AMOR ? &BM4_feliz : expr == E_PARPADEO ? &BM4_parp : expr == E_BATERIA ? &BM4_bat : &BM4_notif;
  else mb = expr == E_FELIZ ? &BM_smile : (expr == E_PARPADEO || expr == E_AMOR) ? &BM_small : expr == E_BATERIA ? &BM_wavy : &BM_o;
  int mw = strlen(mb->r[0]);
  stamp(*mb, (int)lroundf(c.mouthX - mw / 2.0f), c.mouthY + dy, c.dark, c.hi, c.love);

  if (expr == E_BATERIA && sinf(t * 5) > -0.3f) stamp(BM_bat, N / 2 - 3, 1, c.ink, -1, 0xFF3B3B);
  if (expr == E_NOTIF && N >= 16 && fr % 2 == 0) stamp(BM_bang, N - 6, 1, c.accent, -1, -1);

  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++)
      if (G[y][x] >= 0) S->fillRect(x * P, y * P, P, P, (uint32_t)G[y][x]);
}

// ---------------------------------------------------------------- entrada
void renderFace(LGFX_Sprite& s, const Character& c, const FaceCtx& ctx) {
  S = &s;
  X = &ctx;
  BG = c.bg;
  s.fillScreen(c.bg);
  switch (c.type) {
    case T_MINIMAL: minimalFace(c, ctx.expr, ctx.t); break;
    case T_KAWAII: kawaiiFace(c, ctx.expr, ctx.t); break;
    case T_ROBOT: robotFace(c, ctx.expr, ctx.t); break;
    case T_PIXEL: pixelFace(c, ctx.expr, ctx.t); break;
  }
  if (c.type != T_PIXEL || c.N < 16) fx(c, ctx.expr, ctx.t);
  hud(c, ctx.expr, ctx.clock);
  drawTabDots(s, c.fg, c.bg, 0);
}

// ---------------------------------------------------------------- pestana Estado
static void fitText(char* out, size_t cap, const char* in, int maxW) {
  strlcpy(out, in, cap);
  if (S->textWidth(out) <= maxW) return;
  size_t n = strlen(out);
  while (n > 0) {
    out[--n] = 0;
    char tmp[64];
    snprintf(tmp, sizeof tmp, "%s...", out);
    if (S->textWidth(tmp) <= maxW) { strlcpy(out, tmp, cap); return; }
  }
}

void renderStatus(LGFX_Sprite& s, const Character& c, int battery, const Notif* notifs, int n, const char* clock) {
  S = &s;
  BG = c.sBg;
  s.fillScreen(c.sBg);
  const int PAD = 14;

  S->setTextDatum(textdatum_t::top_left);
  S->setFont(&fonts::DejaVu9);
  S->setTextColor(c.sMuted);
  S->drawString("ESTADO", PAD, 14);
  S->setTextDatum(textdatum_t::top_right);
  S->drawString(clock ? clock : "--:--", 240 - PAD, 14);

  // bateria
  uint32_t batCol = battery < 20 ? 0xFF3B3B : c.sFg;
  char pct[8];
  snprintf(pct, sizeof pct, "%d%%", battery);
  S->setFont(&fonts::FreeSansBold18pt7b);
  S->setTextColor(batCol);
  S->setTextDatum(textdatum_t::middle_left);
  S->drawString(pct, PAD, 46);
  int bx = PAD + S->textWidth(pct) + 10, bw = 240 - PAD - 3 - bx;
  rrectStroke(bx, 38, bw, 16, 5, c.sBg, c.sFg, 2);
  rrect(bx + 4, 42, (bw - 8) * constrain(battery, 0, 100) / 100.0f, 8, 2, batCol);
  rrect(bx + bw, 42, 3, 8, 1, c.sFg);

  // notificaciones
  S->setFont(&fonts::DejaVu9);
  S->setTextColor(c.sMuted);
  S->setTextDatum(textdatum_t::top_left);
  char head[32];
  snprintf(head, sizeof head, "NOTIFICACIONES - %d", n);
  S->drawString(head, PAD, 68);

  if (n == 0) {
    S->setTextDatum(textdatum_t::middle_center);
    S->drawString("Sin notificaciones", 120, 130);
  }
  for (int i = 0; i < n && i < 3; i++) {
    const Notif& nt = notifs[i];
    int y = 88 + i * 37;
    rrect(PAD, y, 240 - 2 * PAD, 32, c.rowR, c.row);
    rrect(PAD + 6, y + 5, 22, 22, c.iconR, nt.color);
    S->setFont(&fonts::FreeSansBold9pt7b);
    S->setTextColor((uint32_t)0xFFFFFF);
    S->setTextDatum(textdatum_t::middle_center);
    char ini[2] = {nt.initial, 0};
    S->drawString(ini, PAD + 17, y + 16);

    S->setFont(&fonts::DejaVu9);
    S->setTextDatum(textdatum_t::top_left);
    S->setTextColor(c.sFg);
    S->drawString(nt.from, PAD + 34, y + 3);
    S->setTextColor(c.sMuted);
    S->setTextDatum(textdatum_t::top_right);
    S->drawString(nt.time, 240 - PAD - 6, y + 3);
    S->setTextDatum(textdatum_t::top_left);
    char line[56];
    fitText(line, sizeof line, nt.text, 240 - 2 * PAD - 34 - 6);
    S->drawString(line, PAD + 34, y + 17);
  }
  drawTabDots(s, c.sFg, c.sBg, 1);
}
