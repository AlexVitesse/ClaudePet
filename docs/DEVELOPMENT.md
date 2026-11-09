# Guía de desarrollo

## Estructura del sketch
Carpeta de sketch de Arduino (todos los archivos juntos):

```
ClaudePet/
  ClaudePet.ino         orquestación: setup() + loop()
  config.h              pines, defaults, umbrales, nombres de cabeceras
  secrets_store.*       persistencia NVS (token / modelo / intervalo)
  provisioning.*        portal cautivo WiFiManager (WiFi + token)
  anthropic_client.*    sonda HTTPS + parseo de cabeceras 5h/7d
  pet.*                 máquina de estados de ánimo (con histéresis)
  display.*             render del OLED (caras procedurales + barra)
```

## Responsabilidad de cada módulo

### `ClaudePet.ino`
Orquesta todo. En `loop()`:
- Sondea la API cada `pollSeconds` (`doPoll()`).
- Reintenta WiFi sin bloquear si se cae.
- Anima la cara a ~10 FPS (`display::render`).
- `handleSerial()` procesa comandos de prueba (`r`, `p`, `t`, `x`).

### `anthropic_client.*`
`anthropic::probe(token, model)` → `UsageResult { ok, badToken, used5h, used7d,
reset5h, httpCode }`. Hace `POST /v1/messages` con `max_tokens:1`, registra las
cabeceras con `collectHeaders()` y las lee tras el POST.

### `pet.*`
`pet::stateFromRemaining(remaining, previous)` aplica los umbrales de
`config.h` con histéresis (`STATE_HYSTERESIS`) para no rebotar entre estados.

### `display.*`
Caras **procedurales** con primitivas de U8g2 (no sprites). `drawFace(state,
frame)` decide la forma de los ojos/boca; `render()` añade la barra de % abajo y, si hay dato, la cuenta atrás al reset
con un mini-reloj.
`frame` es un contador que incrementa cada tick (parpadeo, miradas, gota).

### `secrets_store.*` + `provisioning.*`
NVS vía `Preferences`. WiFiManager gestiona las credenciales WiFi; nosotros
persistimos token/modelo/intervalo. El marcador de "provisionado" es el token.

## Ajustes rápidos (qué tocar para...)

| Quiero... | Dónde |
|-----------|-------|
| Cambiar pines del OLED | `config.h` → `OLED_SDA_PIN` / `OLED_SCL_PIN` |
| Cambiar frecuencia de sondeo | `config.h` → `DEFAULT_POLL_SECONDS` (o por portal) |
| Mover los umbrales de ánimo | `config.h` → `TH_HAPPY/NEUTRAL/ANXIOUS` |
| Más/menos histéresis | `config.h` → `STATE_HYSTERESIS` |
| Modelo de la sonda | `config.h` → `DEFAULT_MODEL` (o por portal) |
| Forma de ojos/boca | `display.cpp` → `drawFace()` y helpers `eye*` |
| Velocidad de animación | `ClaudePet.ino` → `ANIM_INTERVAL_MS` |

## Cómo se dibujan las caras
Ejes: pantalla 128×64. Ojos centrados en `cx=64`, `cyEye=22`, separación
`dx=22` (ojos en x≈42 y x≈86). La franja inferior (y 52–62) lleva la barra de
cuota con el % dentro.

Helpers en `display.cpp`:
- `eyeHappy(cx,cy)` — arco tipo `^_^`.
- `eyeOpen(cx,cy,r,blink)` — disco; si `blink`, una línea horizontal.
- `eyeCross(cx,cy,r)` — aspa `X` (pánico/muerto).
- `sweatDrop(x,y)` — gotita (ansioso).

Para añadir frames o una mascota nueva (gato, robot, ojo HAL), basta con
escribir un nuevo `case` o helpers de dibujo; no hay que tocar la lógica de red.

## Pruebas sin gastar cuota
Usa los comandos de serial (`r<NN>`) para recorrer todos los estados sin llamar
a la API. Para probar la red de verdad, usa `p` (sondeo forzado).

## Ideas / roadmap (v2)
- Cert pinning + cifrado AES-256-GCM del token (ver ARCHITECTURE › Seguridad).
- Vista alternativa 7d (botón para alternar 5h ↔ 7d).
- Más mascotas / temas seleccionables.
- Modo batería con deep-sleep configurable.
