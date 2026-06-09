# ClaudePet 🐾

Mascota física de escritorio que muestra cuánta **cuota de Claude (Pro/Max)** te
queda en tu ventana de **5 horas**, con expresiones de ánimo (feliz → pánico) en
una pantalla OLED de 0.96".

El ESP32 es **autónomo**: habla directo con la API de Anthropic por HTTPS, lee
las cabeceras de rate-limit de tu suscripción y mapea la cuota restante a la cara
de la mascota. No necesita PC ni middleware.

```
[ESP32-C3 + OLED SSD1306] --HTTPS cada ~120s--> api.anthropic.com/v1/messages
      |  lee: anthropic-ratelimit-unified-5h-utilization
      v
  máquina de estados -> cara de la mascota + barra de %
```

## Estados de ánimo

| % restante (5h) | Estado        | Cara |
|----------------:|---------------|------|
| 80–100%         | Feliz         | ojitos `^_^`, parpadeo lento |
| 40–79%          | Neutral       | mira de lado a lado |
| 11–39%          | Ansioso       | ojos abiertos, sudando, parpadeo rápido |
| 0–10%           | Pánico        | ojos en `X`, pantalla parpadeando |
| —               | Sin WiFi / Token? | mensajes de error |

## Documentación

Detalle completo en [`docs/`](docs/README.md):
[Arquitectura](docs/ARCHITECTURE.md) · [Hardware](docs/HARDWARE.md) · [Puesta en marcha](docs/SETUP.md) · [Desarrollo](docs/DEVELOPMENT.md)

## Hardware (BOM)

- **ESP32-C3 SuperMini** (o ESP32-WROOM; ver `platformio.ini`).
- **OLED 0.96" SSD1306 128×64 I2C** (4 pines: VCC, GND, SCL, SDA).
- Cable USB para alimentar.

### Cableado (ESP32-C3 SuperMini)

| OLED | ESP32-C3 |
|------|----------|
| VCC  | 3V3      |
| GND  | GND      |
| SDA  | GPIO8    |
| SCL  | GPIO9    |

Ajusta los pines en `src/config.h` (`OLED_SDA_PIN` / `OLED_SCL_PIN`) si tu placa
es distinta.

## Compilar y flashear (Arduino IDE)

### 1. Soporte para placas ESP32
- `Archivo > Preferencias > Gestor de URLs adicionales de tarjetas`, añade:
  `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
- `Herramientas > Placa > Gestor de tarjetas`, busca **esp32** (Espressif) e instala.

### 2. Librerías (Gestor de librerías)
- **U8g2** (de oliver / olikraus)
- **WiFiManager** (de tzapu)

### 3. Abrir y subir
- Abre `ClaudePet.ino` (debe estar en una carpeta llamada `ClaudePet`).
- `Herramientas > Placa`: selecciona tu ESP32-C3 (p. ej. **ESP32C3 Dev Module**
  o **LOLIN C3 Mini**).
- En placas C3, activa **USB CDC On Boot: Enabled** para ver el Serial por USB.
- Selecciona el **Puerto** y pulsa **Subir** (→).
- Abre el **Monitor Serie** a **115200 baudios** para ver los logs.

> Todos los `.cpp/.h` deben estar en la **misma carpeta** que `ClaudePet.ino`
> (el Arduino IDE los compila juntos automáticamente).

## Configuración (portal cautivo)

En el **primer arranque** (o tras un factory reset) el ESP32 crea una red WiFi
`ClaudePet-setup`:

1. Conéctate a esa red desde el móvil/PC.
2. Abre `http://192.168.4.1`.
3. Elige tu WiFi, pega tu **Token OAuth** (ver abajo) y, si quieres, ajusta el
   modelo y el intervalo de sondeo (default 120 s).
4. Guarda. El ESP32 se reinicia y empieza a mostrar la cuota.

### Cómo obtener tu Token OAuth de Claude

El token lo emite tu sesión de **Claude Code** (plan Pro/Max). En Windows:

- Mira `%USERPROFILE%\.claude\.credentials.json` y copia el valor de
  `accessToken` (empieza por `sk-ant-oat...`).
- Si no está ahí, puede estar en el **Administrador de credenciales** de Windows
  (entrada de Claude Code) según tu instalación.

> ⚠️ Trata ese token como una contraseña. Da acceso a tu cuenta de Claude.
> Si caduca, la mascota mostrará el estado `TOKEN?`; vuelve a generarlo y
> reconfigura por el portal.

## Pruebas por puerto serie

Desde el Monitor Serie puedes escribir:

Con el **Monitor Serie** abierto (115200, fin de línea "Nueva línea"):

| Comando | Efecto |
|---------|--------|
| `r5`    | Simula 5% restante (entra en Pánico) |
| `r60`   | Simula 60% (Neutral) |
| `r-`    | Desactiva la simulación |
| `p`     | Fuerza un sondeo a la API ahora |
| `x`     | Factory reset (borra WiFi y token) |

En cada sondeo verás en el log:
`[poll] used5h=NN% used7d=NN% -> restante=NN% estado=...`

## Consumo

- El sondeo por defecto cada **120 s** ⇒ ~30 requests/h, con `max_tokens:1`
  (coste despreciable de tu ventana). Súbelo en el portal si quieres menos red.
- Entre sondeos solo corre la animación (~10 FPS), sin tocar WiFi.

## Notas de seguridad

- **v1 (este código):** TLS con `setInsecure()` (no valida el certificado de
  Anthropic) para ahorrar RAM, y token guardado en NVS en claro. Suficiente para
  uso doméstico en tu red.
- **Endurecimiento (v2):** pinear el certificado raíz de Anthropic
  (`client.setCACert(...)`) y cifrar el token con AES-256-GCM derivado de un PIN.

## Estructura

Carpeta de sketch de Arduino (todos los archivos juntos):

```
ClaudePet/
  ClaudePet.ino         orquestación (setup/loop)
  config.h              pines, defaults, nombres de cabeceras
  secrets_store.*       persistencia NVS (token/modelo/intervalo)
  provisioning.*        portal cautivo WiFiManager
  anthropic_client.*    sonda HTTPS + parseo de cabeceras 5h/7d
  pet.*                 máquina de estados de ánimo (con histéresis)
  display.*             render del OLED (caras procedurales + barra)
```

## Proyectos de referencia / inspiración

- [oauramos/claude-usage-stick](https://github.com/oauramos/claude-usage-stick) — mismo enfoque autónomo.
- [dorofino/ClaudeGauge](https://github.com/dorofino/ClaudeGauge)
- [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy) — mascota oficial (ideas de animaciones).
- [rootedlab-code/claude-code-usage-monitor](https://github.com/rootedlab-code/claude-code-usage-monitor) — variante middleware (JSONL).
