// display.cpp — Render procedural de las caras (sin sprites externos: más ligero
// y robusto que mantener arrays XBM). El "ojo" de la mascota cambia de forma y
// ritmo según el estado de ánimo.
#include "display.h"
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"
#include "pet.h"

// Constructor HW I2C con pines explícitos (clock=SCL, data=SDA).
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /*reset=*/U8X8_PIN_NONE,
                                                /*clock=*/OLED_SCL_PIN,
                                                /*data=*/OLED_SDA_PIN);

namespace {

// Dibuja un ojo "feliz" (arco hacia arriba ^_^) centrado en (cx,cy).
void eyeHappy(int cx, int cy) {
  for (int i = -6; i <= 6; i++) {
    int y = cy + (abs(i) / 2) - 3;  // forma de "v" invertida suave
    u8g2.drawPixel(cx + i, y);
    u8g2.drawPixel(cx + i, y + 1);
  }
}

// Ojo redondo abierto; blink=true lo dibuja como línea (parpadeo).
void eyeOpen(int cx, int cy, int r, bool blink) {
  if (blink) {
    u8g2.drawHLine(cx - r, cy, 2 * r + 1);
  } else {
    u8g2.drawDisc(cx, cy, r);
  }
}

// Ojo "X" (muerto/pánico).
void eyeCross(int cx, int cy, int r) {
  u8g2.drawLine(cx - r, cy - r, cx + r, cy + r);
  u8g2.drawLine(cx - r, cy + r, cx + r, cy - r);
}

// Gotita de sudor (estado ansioso).
void sweatDrop(int x, int y) {
  u8g2.drawDisc(x, y + 2, 2);
  u8g2.drawTriangle(x - 2, y + 2, x + 2, y + 2, x, y - 2);
}

void drawFace(PetState s, uint32_t frame) {
  const int cyEye = 22;     // línea de los ojos
  const int dx = 22;        // separación ojos respecto al centro
  const int cx = 64;
  // Parpadeo: breve cada ~2-3s; en ansioso parpadea más rápido.
  bool blink = false;

  switch (s) {
    case PetState::Happy: {
      // Mirada tranquila; parpadeo lento ocasional.
      blink = (frame % 40) == 0;
      if (blink) { eyeOpen(cx - dx, cyEye, 5, true); eyeOpen(cx + dx, cyEye, 5, true); }
      else       { eyeHappy(cx - dx, cyEye);         eyeHappy(cx + dx, cyEye); }
      // boca sonriente
      u8g2.drawLine(cx - 8, 36, cx, 40);
      u8g2.drawLine(cx, 40, cx + 8, 36);
      break;
    }
    case PetState::Neutral: {
      // Mira de lado a lado.
      int look = ((frame / 8) % 3) - 1;  // -1,0,1
      blink = (frame % 30) == 0;
      eyeOpen(cx - dx + look * 2, cyEye, 5, blink);
      eyeOpen(cx + dx + look * 2, cyEye, 5, blink);
      u8g2.drawHLine(cx - 6, 38, 12);  // boca neutra
      break;
    }
    case PetState::Anxious: {
      // Ojos bien abiertos, parpadeo rápido, sudando.
      blink = (frame % 12) == 0;
      eyeOpen(cx - dx, cyEye, 6, blink);
      eyeOpen(cx + dx, cyEye, 6, blink);
      // pupilas tensas
      if (!blink) { u8g2.setDrawColor(0); u8g2.drawDisc(cx - dx, cyEye, 2);
                    u8g2.drawDisc(cx + dx, cyEye, 2); u8g2.setDrawColor(1); }
      sweatDrop(cx + dx + 9, cyEye - 6 + (int)((frame / 4) % 4));  // gota cayendo
      u8g2.drawLine(cx - 7, 39, cx + 7, 39);  // boca tensa ondulada
      u8g2.drawPixel(cx, 38);
      break;
    }
    case PetState::Panic: {
      // Ojos en X, parpadeo de "alarma" (cara entera invertida).
      bool flash = (frame % 6) < 3;
      if (flash) { u8g2.setDrawColor(1); u8g2.drawBox(0, 0, 128, 50);
                   u8g2.setDrawColor(0); }
      eyeCross(cx - dx, cyEye, 6);
      eyeCross(cx + dx, cyEye, 6);
      u8g2.drawLine(cx - 7, 40, cx + 7, 36);  // boca caída
      u8g2.setDrawColor(1);
      break;
    }
    case PetState::Disconnected: {
      eyeOpen(cx - dx, cyEye, 5, true);
      eyeOpen(cx + dx, cyEye, 5, true);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(cx - 24, 44, "sin wifi");
      break;
    }
    case PetState::BadToken: {
      eyeOpen(cx - dx, cyEye, 5, false);
      eyeOpen(cx + dx, cyEye, 5, false);
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(cx - 30, 44, "token ?");
      break;
    }
  }
}

}  // namespace

namespace display {

void begin() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  u8g2.begin();
  u8g2.setContrast(120);  // un punto menos de brillo = menos consumo
}

void splash(const char* line1, const char* line2) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x13B_tf);
  u8g2.drawStr(2, 24, line1);
  if (line2) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(2, 42, line2);
  }
  u8g2.sendBuffer();
}

void render(PetState state, int remaining, uint32_t frame) {
  u8g2.clearBuffer();

  drawFace(state, frame);

  // Franja inferior: barra de cuota + % + estado.
  if (remaining >= 0) {
    int w = map(remaining, 0, 100, 0, 124);
    u8g2.drawFrame(2, 52, 124, 10);
    if (w > 0) u8g2.drawBox(2, 52, w, 10);

    char buf[20];
    snprintf(buf, sizeof(buf), "%d%%", remaining);
    u8g2.setFont(u8g2_font_5x8_tf);
    // texto del % en blanco/negro según relleno de la barra
    int tx = 56;
    u8g2.setDrawColor(w > 64 ? 0 : 1);
    u8g2.drawStr(tx, 60, buf);
    u8g2.setDrawColor(1);
  } else {
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(2, 60, pet::name(state));
  }

  u8g2.sendBuffer();
}

}  // namespace display
