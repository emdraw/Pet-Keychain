#pragma once
#include <Arduino.h>
#include "display.h"

enum Expr { E_FELIZ, E_PARPADEO, E_AMOR, E_BATERIA, E_NOTIF, E_COUNT };
enum Type { T_MINIMAL, T_KAWAII, T_ROBOT, T_PIXEL };
enum Shape { S_NONE, S_MOCHI, S_NUBE, S_GATO, S_PERRO };          // kawaii
enum Body { B_NONE, B_BLOB, B_GHOST, B_CAT, B_DOG, B_SKULL, B_TREE };              // pixel con cuerpo
enum Flat { F_NONE, F_CAT, F_DOG };                               // pixel plano 12x12
enum Nose { N_NONE, N_CAT, N_DOG };                               // kawaii

#define NOCOL 0xFFFFFFFFu

struct Character {
  const char* id = "";
  const char* name = "";
  const char* style = "";
  Type type = T_MINIMAL;
  uint8_t shape = 0;       // kawaii: Shape | robot: 0 bolt, 1 antena
  uint8_t body = 0;        // Body
  uint8_t flat = 0;        // Flat
  uint8_t nose = 0;        // Nose (kawaii)
  bool glow = false, noMouth = false;
  uint8_t N = 0;           // celdas por lado (pixel)
  int16_t eyeY = 0, eyeW = 0, eyeH = 0, eyeR = 0, mouthDy = 0;
  int8_t eyes[2][2] = {{0, 0}, {0, 0}};
  int8_t noseCells[2][2] = {{0, 0}, {0, 0}};
  bool hasNoseCells = false;
  int8_t mouthX = 0, mouthY = 0;
  uint32_t bg = NOCOL, eye = NOCOL, face = NOCOL, blush = NOCOL, mouth = NOCOL, love = NOCOL,
           noseCol = NOCOL, visor = NOCOL, visorStroke = NOCOL, led = NOCOL, plate = NOCOL,
           plateStroke = NOCOL, eyeDark = NOCOL, dark = NOCOL, hi = NOCOL, mid = NOCOL,
           bodyCol = NOCOL, line = NOCOL, inner = NOCOL, ear = NOCOL, muzzle = NOCOL,
           ink = NOCOL, fg = NOCOL, accent = NOCOL, shell = NOCOL;
  // pantalla "Estado"
  uint32_t sBg = NOCOL, sFg = NOCOL, sMuted = NOCOL, row = NOCOL;
  uint8_t rowR = 0, iconR = 0;
};

struct Notif {
  char initial = '?';
  char from[16] = "";
  char text[48] = "";
  char time[6] = "--:--";
  uint32_t color = 0x2FB36B;
};

// Todo lo que el dibujo de la cara necesita saber del mundo exterior
struct FaceCtx {
  int expr = E_FELIZ;
  float t = 0;
  const char* clock = nullptr;  // "HH:MM" o nullptr
  int notifCount = 0;
  uint32_t notifCol[3] = {0, 0, 0};
};

void initCharacters();
int characterCount();
const Character& characterAt(int i);
int characterIndex(const char* id);  // -1 si no existe

void renderFace(LGFX_Sprite& s, const Character& c, const FaceCtx& ctx);
void drawTabDots(LGFX_Sprite& s, uint32_t color, uint32_t bg, int active);  // 3 puntos: cara, estado, red
void renderStatus(LGFX_Sprite& s, const Character& c, int battery, const Notif* notifs, int n, const char* clock);
