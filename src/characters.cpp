#include "faces.h"

// Paletas y parametros copiados de UI/design_handoff_llavero_amigo (arrays CH y CH2).
static Character CH[20];
static int count = 0;

static Character& add(const char* id, const char* name, const char* style, Type type) {
  Character& c = CH[count++];
  c = Character();
  c.id = id;
  c.name = name;
  c.style = style;
  c.type = type;
  return c;
}

static void applyStatusPalettes();

void initCharacters() {
  if (count) return;
  Character* c;

  // ---- Turno 1 ----
  c = &add("1a", "Lumi", "Ojos minimalistas", T_MINIMAL);
  c->bg = 0x05070A; c->eye = 0x7DF9FF; c->love = 0xFF5C8A; c->glow = true;
  c->eyeW = 54; c->eyeH = 74; c->eyeR = 22; c->shell = 0x252B36; c->fg = 0x7DF9FF; c->accent = 0x7DF9FF;

  c = &add("1b", "Miel", "Ojos minimalistas", T_MINIMAL);
  c->bg = 0x140B04; c->eye = 0xFFB23F; c->love = 0xFF6A5C; c->glow = true;
  c->eyeW = 62; c->eyeH = 50; c->eyeR = 25; c->shell = 0x4A3420; c->fg = 0xFFB23F; c->accent = 0xFFB23F;

  c = &add("1c", "Mochi", "Kawaii", T_KAWAII);
  c->shape = S_MOCHI; c->bg = 0xFFC8D8; c->face = 0xFFFFFF; c->eye = 0x2B1B24; c->blush = 0xFF8FB1;
  c->mouth = 0xC2385F; c->love = 0xFF3B6B; c->shell = 0xFF9DB9; c->fg = 0x7A2E48; c->accent = 0xFF5C8A; c->eyeY = 142;

  c = &add("1d", "Nimbo", "Kawaii", T_KAWAII);
  c->shape = S_NUBE; c->bg = 0x8EC9FF; c->face = 0xFFFFFF; c->eye = 0x1D2A44; c->blush = 0xFFB0C8;
  c->mouth = 0xE2557A; c->love = 0xFF3B6B; c->shell = 0xBFE0FF; c->fg = 0x1D2A44; c->accent = 0x3D8BFF; c->eyeY = 140;

  c = &add("1e", "Michi", "Kawaii", T_KAWAII);
  c->shape = S_GATO; c->bg = 0xFFF1D6; c->face = 0xF7A55A; c->eye = 0x3A2412; c->blush = 0xFF7F7F;
  c->mouth = 0x3A2412; c->love = 0xE8314F; c->shell = 0xF7A55A; c->fg = 0x3A2412; c->accent = 0xF07E2A; c->eyeY = 142;

  c = &add("1f", "Bolt", "Robot", T_ROBOT);
  c->shape = 0; c->bg = 0x0C1430; c->visor = 0x03060F; c->visorStroke = 0x22356B; c->eye = 0x3CF0FF;
  c->led = 0x3CF0FF; c->love = 0xFF4D8D; c->glow = true; c->shell = 0x2B3A66; c->fg = 0x3CF0FF; c->accent = 0x3CF0FF;

  c = &add("1g", "Tuerca", "Robot", T_ROBOT);
  c->shape = 1; c->bg = 0xC9CFD6; c->plate = 0xEEF1F4; c->plateStroke = 0xAEB6C0; c->eyeDark = 0x20242B;
  c->led = 0xFF6A13; c->love = 0xE8314F; c->shell = 0x8F99A5; c->fg = 0x1E232A; c->accent = 0xFF6A13;

  c = &add("1h", "Pix", "Pixel art", T_PIXEL);
  c->N = 12; c->bg = 0x9BBC0F; c->dark = 0x0F380F; c->hi = 0xCADC9F; c->love = 0xE8F5B0; c->ink = 0x0F380F;
  c->eyes[0][0] = 2; c->eyes[0][1] = 4; c->eyes[1][0] = 7; c->eyes[1][1] = 4; c->mouthX = 6; c->mouthY = 8;
  c->shell = 0xC9C7BF; c->fg = 0x0F380F; c->accent = 0x0F380F;

  c = &add("1i", "Slimo", "Pixel art", T_PIXEL);
  c->N = 20; c->body = B_BLOB; c->bg = 0x170E2B; c->bodyCol = 0x9B6BFF; c->line = 0x3A1F7A; c->dark = 0x1B0B3A;
  c->hi = 0xE3D6FF; c->love = 0xFF4D8D; c->ink = 0xEFE6FF; c->blush = 0xFF8FC0;
  c->eyes[0][0] = 5; c->eyes[0][1] = 10; c->eyes[1][0] = 12; c->eyes[1][1] = 10; c->mouthX = 10; c->mouthY = 14;
  c->shell = 0x5B3BA8; c->fg = 0xEFE6FF; c->accent = 0xFF4D8D;

  c = &add("1j", "Boo", "Pixel art", T_PIXEL);
  c->N = 20; c->body = B_GHOST; c->bg = 0x0F1B33; c->bodyCol = 0xF3F5FF; c->line = 0x7D8CB5; c->dark = 0x0F1B33;
  c->hi = 0xFFFFFF; c->love = 0xFF5C7A; c->ink = 0xF3F5FF; c->blush = 0xFFB3C1;
  c->eyes[0][0] = 5; c->eyes[0][1] = 8; c->eyes[1][0] = 12; c->eyes[1][1] = 8; c->mouthX = 10; c->mouthY = 12;
  c->shell = 0x31466F; c->fg = 0xF3F5FF; c->accent = 0xA9C1FF;

  // ---- Turno 2 ----
  Character bolt = CH[5], michi = CH[4];

  CH[count] = bolt; c = &CH[count++];
  c->id = "2a"; c->style = "Robot - sin boca"; c->noMouth = true;

  CH[count] = michi; c = &CH[count++];
  c->id = "2b"; c->style = "Kawaii - con nariz"; c->nose = N_CAT; c->noseCol = 0xE86A7A;

  c = &add("2c", "Toby", "Kawaii - perrito", T_KAWAII);
  c->shape = S_PERRO; c->bg = 0xDDF0E3; c->face = 0xE8B57E; c->ear = 0x8B5A2B; c->muzzle = 0xFFF1DE;
  c->eye = 0x2B1A10; c->blush = 0xFF9E9E; c->mouth = 0x2B1A10; c->love = 0xE8314F; c->nose = N_DOG;
  c->noseCol = 0x2B1A10; c->mouthDy = 6; c->shell = 0xC98A4B; c->fg = 0x2B1A10; c->accent = 0xE07A2E; c->eyeY = 132;

  c = &add("2d", "Michi", "8 bits", T_PIXEL);
  c->N = 20; c->body = B_CAT; c->bg = 0x1F2A3A; c->bodyCol = 0xF7A55A; c->inner = 0xFF9EB0; c->line = 0x8A4A16;
  c->dark = 0x2B160A; c->hi = 0xFFE2BF; c->love = 0xFF4D6D; c->blush = 0xFF8F8F; c->ink = 0xFFE9D2;
  c->eyes[0][0] = 5; c->eyes[0][1] = 10; c->eyes[1][0] = 12; c->eyes[1][1] = 10;
  c->hasNoseCells = true; c->noseCells[0][0] = 9; c->noseCells[0][1] = 13; c->noseCells[1][0] = 10; c->noseCells[1][1] = 13;
  c->noseCol = 0xFF6F91; c->mouthX = 10; c->mouthY = 14; c->shell = 0xC9772F; c->fg = 0xFFE9D2; c->accent = 0xFF9E3D;

  c = &add("2e", "Toby", "8 bits", T_PIXEL);
  c->N = 20; c->body = B_DOG; c->bg = 0x12303A; c->bodyCol = 0xE3B178; c->ear = 0x8B5A2B; c->muzzle = 0xFFF0DC;
  c->line = 0x4A2A10; c->dark = 0x2B1A10; c->hi = 0xFFFFFF; c->love = 0xFF4D6D; c->blush = 0xFF9E9E; c->ink = 0xE9F4F6;
  c->eyes[0][0] = 5; c->eyes[0][1] = 9; c->eyes[1][0] = 12; c->eyes[1][1] = 9;
  c->hasNoseCells = true; c->noseCells[0][0] = 9; c->noseCells[0][1] = 13; c->noseCells[1][0] = 10; c->noseCells[1][1] = 13;
  c->noseCol = 0x2B1A10; c->mouthX = 10; c->mouthY = 15; c->shell = 0x8B5A2B; c->fg = 0xE9F4F6; c->accent = 0xFFB547;

  c = &add("2f", "Michi", "Pixel art", T_PIXEL);
  c->N = 12; c->flat = F_CAT; c->bg = 0xF4C27A; c->dark = 0x4A2511; c->mid = 0xB5652B; c->hi = 0xFFE6B8;
  c->love = 0xB0263A; c->ink = 0x4A2511;
  c->eyes[0][0] = 2; c->eyes[0][1] = 4; c->eyes[1][0] = 7; c->eyes[1][1] = 4;
  c->hasNoseCells = true; c->noseCells[0][0] = 5; c->noseCells[0][1] = 7; c->noseCells[1][0] = 6; c->noseCells[1][1] = 7;
  c->noseCol = 0xB0263A; c->mouthX = 6; c->mouthY = 8; c->shell = 0xD9A35A; c->fg = 0x4A2511; c->accent = 0x4A2511;

  c = &add("2g", "Toby", "Pixel art", T_PIXEL);
  c->N = 12; c->flat = F_DOG; c->bg = 0xBFD7EA; c->dark = 0x1E2F44; c->mid = 0x7A5233; c->hi = 0xFFFFFF;
  c->love = 0xD6336C; c->ink = 0x1E2F44;
  c->eyes[0][0] = 2; c->eyes[0][1] = 4; c->eyes[1][0] = 7; c->eyes[1][1] = 4;
  c->hasNoseCells = true; c->noseCells[0][0] = 5; c->noseCells[0][1] = 7; c->noseCells[1][0] = 6; c->noseCells[1][1] = 7;
  c->noseCol = 0x1E2F44; c->mouthX = 6; c->mouthY = 8; c->shell = 0x8FB3CF; c->fg = 0x1E2F44; c->accent = 0x1E2F44;

  // ---- Turno 3 ----
  c = &add("3a", "Craneo", "8 bits", T_PIXEL);
  c->N = 20; c->body = B_SKULL; c->bg = 0x1B1230; c->bodyCol = 0xEDE4C8; c->line = 0x5E5640; c->dark = 0x1B1230;
  c->hi = 0xFFB347; c->love = 0xFF4D6D; c->ink = 0xEDE4C8;
  c->eyes[0][0] = 5; c->eyes[0][1] = 8; c->eyes[1][0] = 12; c->eyes[1][1] = 8;
  c->hasNoseCells = true; c->noseCells[0][0] = 9; c->noseCells[0][1] = 12; c->noseCells[1][0] = 10; c->noseCells[1][1] = 12;
  c->noseCol = 0x1B1230; c->mouthX = 10; c->mouthY = 15; c->shell = 0x5E5640; c->fg = 0xEDE4C8; c->accent = 0xFF6A13;

  c = &add("3b", "Arbolito", "8 bits - Navidad", T_PIXEL);
  c->N = 20; c->body = B_TREE; c->bg = 0x0B1D3A; c->bodyCol = 0x2FA84F; c->line = 0x0F4D24; c->dark = 0x0B2A14;
  c->hi = 0xFFFFFF; c->love = 0xFF4D6D; c->ink = 0xFFF4D6; c->blush = 0xFF8FA3; c->inner = 0xF5D547; c->ear = 0x8B5A2B;
  c->eyes[0][0] = 6; c->eyes[0][1] = 8; c->eyes[1][0] = 11; c->eyes[1][1] = 8; c->mouthX = 10; c->mouthY = 12;
  c->shell = 0x1F7A3A; c->fg = 0xFFF4D6; c->accent = 0xF5D547;

  applyStatusPalettes();
}

struct StatusPal { const char* id; uint32_t sBg, sFg, sMuted, row; uint8_t rowR, iconR; };
static const StatusPal STATUS_PAL[] = {
    {"1a", 0x05070A, 0xE8FDFF, 0x7A979E, 0x0F1A1F, 8, 11},  {"1b", 0x140B04, 0xFFE9C7, 0xB08D62, 0x2A1A0B, 8, 11},
    {"1c", 0xFFF5F8, 0x3A1F2B, 0x9A6676, 0xFFE3EC, 12, 11}, {"1d", 0xEAF5FF, 0x1D2A44, 0x5F7799, 0xD5E9FF, 12, 11},
    {"1e", 0xFFF7E8, 0x3A2412, 0x8F6C4A, 0xFFE7C2, 12, 11}, {"1f", 0x0C1430, 0xDDF8FF, 0x7F95C4, 0x142048, 4, 4},
    {"1g", 0xE9ECEF, 0x1E232A, 0x5E6672, 0xD3D8DE, 4, 4},   {"1h", 0x9BBC0F, 0x0F380F, 0x306230, 0x8BAC0F, 0, 0},
    {"1i", 0x170E2B, 0xEFE6FF, 0x9C8AC9, 0x261747, 0, 0},   {"1j", 0x0F1B33, 0xF3F5FF, 0x8F9DC4, 0x1A2A4C, 0, 0},
    {"2a", 0x0C1430, 0xDDF8FF, 0x7F95C4, 0x142048, 4, 4},   {"2b", 0xFFF7E8, 0x3A2412, 0x8F6C4A, 0xFFE7C2, 12, 11},
    {"2c", 0xF3FAF5, 0x2B1A10, 0x7B6A58, 0xDDEFE2, 12, 11}, {"2d", 0x1F2A3A, 0xFFE9D2, 0xA89886, 0x2C3A4F, 0, 0},
    {"2e", 0x12303A, 0xE9F4F6, 0x8CABB3, 0x1B4250, 0, 0},   {"2f", 0xF4C27A, 0x4A2511, 0x7A3F18, 0xEBB168, 0, 0},
    {"2g", 0xBFD7EA, 0x1E2F44, 0x455F7A, 0xA9C6DD, 0, 0},
    {"3a", 0x1B1230, 0xEDE4C8, 0x8F86A8, 0x2A1F45, 0, 0},   {"3b", 0x0B2A18, 0xFFF4D6, 0x8FB59A, 0x123D24, 0, 0}};

static void applyStatusPalettes() {
  for (int i = 0; i < count; i++)
    for (const auto& p : STATUS_PAL)
      if (strcmp(p.id, CH[i].id) == 0) {
        CH[i].sBg = p.sBg; CH[i].sFg = p.sFg; CH[i].sMuted = p.sMuted; CH[i].row = p.row;
        CH[i].rowR = p.rowR; CH[i].iconR = p.iconR;
      }
}

int characterCount() { return count; }
const Character& characterAt(int i) { return CH[i]; }
int characterIndex(const char* id) {
  for (int i = 0; i < count; i++)
    if (strcmp(CH[i].id, id) == 0) return i;
  return -1;
}
