# Hardware y cableado

## Lista de materiales (BOM)
| Componente | Notas |
|-----------|-------|
| **ESP32-C3 SuperMini** | Recomendado: barato, WiFi, ~400 KB RAM (suficiente para TLS), bajo consumo. Alternativa: ESP32-WROOM (`esp32dev`). |
| **OLED 0.96" SSD1306 128×64 I2C** | Monocromo, 4 pines (VCC, GND, SCL, SDA). |
| Cable USB | Para alimentar y flashear. |
| (Opcional) Botón pulsador | Para forzar factory reset al arranque. |

## Cableado (ESP32-C3 SuperMini)
| OLED | ESP32-C3 | Pin en `config.h` |
|------|----------|-------------------|
| VCC  | 3V3      | — |
| GND  | GND      | — |
| SDA  | GPIO8    | `OLED_SDA_PIN` |
| SCL  | GPIO9    | `OLED_SCL_PIN` |

```
   ESP32-C3 SuperMini            OLED SSD1306 0.96"
   ┌──────────────┐              ┌───────────────┐
   │          3V3 ├──────────────┤ VCC           │
   │          GND ├──────────────┤ GND           │
   │       GPIO8  ├──────────────┤ SDA           │
   │       GPIO9  ├──────────────┤ SCL           │
   │          USB │              └───────────────┘
   └──────┬───────┘
          │ a PC / cargador 5V
```

## Cambiar de placa o de pines
- **Otros pines I2C:** edita `OLED_SDA_PIN` / `OLED_SCL_PIN` en `config.h`.
- **ESP32-WROOM clásico:** los pines I2C por defecto suelen ser `SDA=21`,
  `SCL=22`. Ajusta `config.h` y selecciona la placa adecuada en el IDE.
- **Botón opcional:** pon `BUTTON_PIN` a un GPIO libre con pull-up; al arrancar
  con el botón pulsado se hace factory reset. Por defecto está desactivado (-1)
  para no chocar con el I2C.

## Consumo aproximado
- El cuello de botella de energía es el WiFi durante el sondeo. Con sondeo cada
  120 s, el grueso del tiempo el radio está ocioso y solo corre la animación.
- Pensado para estar **enchufado por USB** 24/7. Para batería habría que añadir
  deep-sleep (no incluido en v1, porque reconectar WiFi en cada ciclo gasta más
  de lo que ahorra para esta señal de cambio lento).
