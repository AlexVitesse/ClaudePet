// display.h — Wrapper del OLED SSD1306 128x64 (U8g2) y render de la mascota.
#pragma once
#include <Arduino.h>
#include "pet.h"

namespace display {
// Inicializa el OLED por I2C (usa los pines de config.h).
void begin();

// Pantalla de arranque / mensaje de estado de una línea.
void splash(const char* line1, const char* line2 = nullptr);

// Dibuja la cara de la mascota + barra y % restante.
//   state    : estado de ánimo
//   remaining: % restante (0..100), -1 si desconocido (oculta barra)
//   frame    : contador de animación (incrementa cada tick para parpadeo/movim.)
//   resetText: cadena corta a mostrar junto a la barra (p.ej. "3h12m" o "14:32"). Pasa nullptr o "" para ocultarla.
void render(PetState state, int remaining, uint32_t frame, const char* resetText = nullptr);
}  // namespace display
