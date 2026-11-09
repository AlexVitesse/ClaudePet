// display.cpp — Render procedural del avatar (logo de Claude Code) sobre el OLED.
// El cuerpo es el glifo de bloques de Claude Code, decodificado a un mapa de bits
// 16x5 y escalado. Encima se dibujan ojos/boca y micro-movimiento que cambian
// según el estado de ánimo (que a su vez depende de la cuota restante).
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

// --- Mapa de bits del logo de Claude Code (16 ancho x 5 alto) -----------------
// Obtenido del arte de bloques Unicode del banner de Claude Code. Cada fila es
// una máscara de 16 bits; el bit 0 es la columna más a la izquierda.
//   row0  ..############..
//   row1  ..##.######.##..   <- muescas (x=4 y x=11) = zona de ojos
//   row2  ################
//   row3  ..############..
//   row4  ...#.#....#.#...   <- patitas
static const uint16_t CLAWD[5] = {0x3FFC, 0x37EC, 0xFFFF, 0x3FFC, 0x1428};
static const int CLAWD_W = 16;
static const int CLAWD_H = 5;

// Gotita de sudor (estado ansioso).
void sweatDrop(int x, int y) {
  u8g2.drawDisc(x, y + 2, 2);
  u8g2.drawTriangle(x - 2, y + 2, x + 2, y + 2, x, y - 2);
}

// Dibuja el logo escalado en (x0,y0) con factor s. La fila de patitas (row4)
// recibe un desplazamiento horizontal `legDx` para animar el "caminar".
void drawClawdBody(int x0, int y0, int s, int legDx) {
  for (int row = 0; row < CLAWD_H; row++) {
    uint16_t bits = CLAWD[row];
    int ry = y0 + row * s;
    int dx = (row == 4) ? legDx : 0;
    for (int col = 0; col < CLAWD_W; col++) {
      if (bits & (1u << col)) {
        u8g2.drawBox(x0 + col * s + dx, ry, s, s);
      }
    }
  }
}

void drawFace(PetState s, uint32_t frame) {
  const int S  = 5;                       // escala del logo
  const int bw = CLAWD_W * S;             // ancho del cuerpo = 80
  const int bx = (128 - bw) / 2;          // centrado horizontal = 24
  const int by = 6;                       // borde superior del cuerpo

  // --- Offsets de movimiento por estado ---
  int ox = 0, oy = 0, legDx = 0;
  bool flash = false;

  if (s == PetState::Happy) {
    oy = (int)(sin(frame * 0.3) * 2.0);          // rebota contento
    legDx = (frame % 8 < 4) ? 2 : -2;            // patitas bailan
  } else if (s == PetState::Neutral) {
    oy = (frame / 10) % 2;                        // rebote suave
    legDx = (frame % 12 < 6) ? 1 : -1;
  } else if (s == PetState::Anxious) {
    ox = (frame % 2 == 0) ? -1 : 1;              // tiembla
  } else if (s == PetState::Panic) {
    ox = (frame % 2 == 0) ? -2 : 2;              // sacudida fuerte
    flash = (frame % 6) < 3;
  }

  // --- Cuerpo (logo) ---
  drawClawdBody(bx + ox, by + oy, S, legDx);

  // --- Coordenadas de la cara (alineadas con las muescas del logo) ---
  const int eyeLX = bx + 4  * S + S / 2 + ox;    // ~46
  const int eyeRX = bx + 11 * S + S / 2 + ox;    // ~81
  const int eyeY  = by + 2  * S + oy;            // ~16
  const int mx    = bx + bw / 2 + ox;            // ~64
  const int my    = by + 3  * S + oy + 1;        // ~22

  bool blink = (s == PetState::Happy   && frame % 40 == 0) ||
               (s == PetState::Neutral && frame % 35 == 0);

  // Ojos y boca van en NEGRO, recortados sobre el cuerpo blanco.
  u8g2.setDrawColor(0);

  // --- Ojos ---
  if (s == PetState::Panic) {
    // Ojos en X (alarma)
    u8g2.drawLine(eyeLX - 3, eyeY - 3, eyeLX + 3, eyeY + 3);
    u8g2.drawLine(eyeLX - 3, eyeY + 3, eyeLX + 3, eyeY - 3);
    u8g2.drawLine(eyeRX - 3, eyeY - 3, eyeRX + 3, eyeY + 3);
    u8g2.drawLine(eyeRX - 3, eyeY + 3, eyeRX + 3, eyeY - 3);
  } else if (s == PetState::Happy) {
    // Ojitos felices ^_^
    u8g2.drawLine(eyeLX - 3, eyeY + 1, eyeLX, eyeY - 2);
    u8g2.drawLine(eyeLX, eyeY - 2, eyeLX + 3, eyeY + 1);
    u8g2.drawLine(eyeRX - 3, eyeY + 1, eyeRX, eyeY - 2);
    u8g2.drawLine(eyeRX, eyeY - 2, eyeRX + 3, eyeY + 1);
  } else if (blink) {
    // Parpadeo
    u8g2.drawHLine(eyeLX - 3, eyeY, 7);
    u8g2.drawHLine(eyeRX - 3, eyeY, 7);
  } else if (s == PetState::Anxious) {
    // Ojos muy abiertos
    u8g2.drawDisc(eyeLX, eyeY, 3);
    u8g2.drawDisc(eyeRX, eyeY, 3);
  } else if (s == PetState::Disconnected || s == PetState::BadToken) {
    // Ojos caídos / tristes
    u8g2.drawLine(eyeLX - 3, eyeY - 2, eyeLX + 3, eyeY + 2);
    u8g2.drawLine(eyeRX - 3, eyeY + 2, eyeRX + 3, eyeY - 2);
  } else {
    // Neutral: ojos redondos
    u8g2.drawDisc(eyeLX, eyeY, 2);
    u8g2.drawDisc(eyeRX, eyeY, 2);
  }

  // --- Boca ---
  if (s == PetState::Happy) {
    u8g2.drawLine(mx - 3, my, mx, my + 2);       // sonrisa
    u8g2.drawLine(mx, my + 2, mx + 3, my);
  } else if (s == PetState::Anxious) {
    u8g2.drawLine(mx - 3, my, mx + 3, my);       // tensa
  } else if (s == PetState::Panic) {
    u8g2.drawDisc(mx, my, 2);                     // 'O' de susto
  } else if (s == PetState::Disconnected || s == PetState::BadToken) {
    u8g2.drawLine(mx - 3, my + 1, mx, my - 1);   // triste
    u8g2.drawLine(mx, my - 1, mx + 3, my + 1);
  } else {
    u8g2.drawHLine(mx - 2, my, 5);               // neutral
  }

  u8g2.setDrawColor(1);  // restaurar blanco

  // --- Gota de sudor (ansioso), en blanco junto al cuerpo ---
  if (s == PetState::Anxious) {
    sweatDrop(bx + bw + 4 + ox, by + 4 + (int)((frame / 3) % 5));
  }

  // --- Parpadeo de pantalla en pánico ---
  if (s == PetState::Panic && flash) {
    u8g2.setDrawColor(2);            // XOR: invierte la zona del avatar
    u8g2.drawBox(0, 0, 128, 48);
    u8g2.setDrawColor(1);
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

void render(PetState state, int remaining, uint32_t frame, const char* resetText) {
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

    // Hora/cuenta atrás de reset, esquina inferior derecha.
    if (resetText && resetText[0]) {
      u8g2.setFont(u8g2_font_4x6_tf);
      int tw = u8g2.getStrWidth(resetText);
      int rx = 126 - tw;                  // pegado al borde derecho interno

      // Reservamos espacio para un icono de mini-reloj (6 píxeles de ancho)
      int iconSpace = 6;
      rx -= iconSpace;

      // Fondo para legibilidad sobre la barra rellena
      u8g2.setDrawColor(0);
      u8g2.drawBox(rx - 1, 53, tw + iconSpace + 2, 7);
      u8g2.setDrawColor(1);

      // Dibujar mini-reloj: círculo de radio 2 y una manecilla
      int clockX = rx + 2;
      int clockY = 56;
      u8g2.drawCircle(clockX, clockY, 2);
      u8g2.drawPixel(clockX, clockY);
      u8g2.drawPixel(clockX + 1, clockY - 1); // manecilla apuntando arriba-derecha

      u8g2.drawStr(rx + iconSpace, 59, resetText);
    }
  } else {
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(2, 60, pet::name(state));
  }

  u8g2.sendBuffer();
}

}  // namespace display
