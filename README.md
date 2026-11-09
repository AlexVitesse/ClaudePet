# ClaudePet 🐾

> Una mascota física de escritorio que te enseña cuánta **cuota de Claude (Pro/Max)**
> te queda en la ventana de **5 horas**, y cuándo se reinicia.

![ESP32](https://img.shields.io/badge/ESP32-C3%20%7C%20WROOM-E7352C?logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino%20IDE-00878F?logo=arduino&logoColor=white)
![OLED](https://img.shields.io/badge/OLED-SSD1306%20128x64-222)

Un ESP32 con una pantalla OLED de 0.96" que pone cara feliz cuando vas sobrado y
entra en pánico cuando estás a punto de agotar la cuota. Es **autónomo**: habla
directo con la API de Anthropic por HTTPS y lee las cabeceras de rate-limit de tu
suscripción. No necesita PC, ni scraping, ni middleware.

```
[ESP32 + OLED] ──HTTPS cada ~120 s──▶ api.anthropic.com/v1/messages  (max_tokens: 1)
      │  lee: anthropic-ratelimit-unified-5h-utilization
      │       anthropic-ratelimit-unified-5h-reset  (+ cabecera Date)
      ▼
  máquina de estados ──▶ cara de la mascota + barra de % + cuenta atrás al reset
```

## Qué muestra

| % restante (5 h) | Estado   | Cara |
|-----------------:|----------|------|
| 80–100 %         | Feliz    | ojitos `^_^`, parpadeo lento |
| 40–79 %          | Neutral  | mira de lado a lado |
| 11–39 %          | Ansioso  | ojos abiertos, sudando, parpadeo rápido |
| 0–10 %           | Pánico   | ojos en `X`, pantalla parpadeando |
| —                | `SIN WIFI` / `TOKEN?` | mensajes de error |

Abajo aparece una barra con el % restante y la **cuenta atrás hasta el reinicio**
de la ventana (p. ej. `3h12m`). La cuenta atrás se calcula restando la hora del
servidor (cabecera `Date`) al epoch de reset, así que no hace falta NTP.

## Hardware

- **ESP32** — DevKit/WROOM clásico o **ESP32-C3 SuperMini**.
- **OLED 0.96" SSD1306 128×64 I2C** (VCC, GND, SCL, SDA).
- Cable USB para alimentar.

| OLED | ESP32 clásico | ESP32-C3 SuperMini |
|------|---------------|--------------------|
| VCC  | 3V3           | 3V3                |
| GND  | GND           | GND                |
| SDA  | GPIO21        | GPIO8              |
| SCL  | GPIO22        | GPIO9              |

Por defecto `config.h` viene para **ESP32 clásico** (21/22). Para el C3 cambia
`OLED_SDA_PIN` / `OLED_SCL_PIN`. Más detalle en [docs/HARDWARE.md](docs/HARDWARE.md).

## Instalación rápida (Arduino IDE)

1. **Placas ESP32:** en `Archivo > Preferencias`, añade
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json` y luego
   instala **esp32** (Espressif) desde el Gestor de tarjetas.
2. **Librerías:** instala **U8g2** (olikraus) y **WiFiManager** (tzapu).
3. **Clona** el repo; la carpeta debe llamarse `ClaudePet` (requisito del IDE):
   ```bash
   git clone https://github.com/AlexVitesse/ClaudePet.git
   ```
4. Abre `ClaudePet.ino`, elige tu placa y puerto, y pulsa **Subir**.
   En placas C3 activa **USB CDC On Boot: Enabled** para ver el Serial.
5. Abre el **Monitor Serie** a **115200** baudios.

## Configuración (portal cautivo)

En el primer arranque el ESP32 crea la red WiFi **`ClaudePet-setup`**:

1. Conéctate a ella y abre `http://192.168.4.1`.
2. Elige tu WiFi y pega tu **token OAuth** (ver abajo). Opcional: modelo e
   intervalo de sondeo (default 120 s).
3. Guarda. El dispositivo se reinicia y empieza a mostrar la cuota.

### Obtener el token OAuth

Necesitas [Claude Code](https://docs.anthropic.com/en/docs/claude-code) con tu
cuenta Pro/Max. Genera un token de larga duración:

```bash
claude setup-token
```

Copia el token que imprime (`sk-ant-oat...`) y pégalo en el portal.

> [!WARNING]
> **No uses** el `accessToken` de `~/.claude/.credentials.json`: Claude Code lo
> rota constantemente y deja de funcionar en minutos.

> [!CAUTION]
> El token da acceso a tu cuenta de Claude: trátalo como una contraseña y nunca lo
> subas a git. Se guarda solo en la memoria NVS del ESP32.

## Comandos por Monitor Serie

Fin de línea: **"Nueva línea"**.

| Comando    | Efecto |
|------------|--------|
| `r5`       | Simula 5 % restante (Pánico) |
| `r60`      | Simula 60 % (Neutral) |
| `r-`       | Desactiva la simulación |
| `p`        | Fuerza un sondeo real ahora |
| `t<TOKEN>` | Reemplaza el token sin pasar por el portal |
| `x`        | Factory reset (borra WiFi y token) |

En cada sondeo verás:
`[poll] used5h=NN% used7d=NN% -> restante=NN% reset=NNNNs estado=...`

## Consumo de cuota

Un sondeo cada 120 s ≈ 30 requests/hora con `max_tokens: 1` sobre Haiku: coste
despreciable. Entre sondeos solo corre la animación (~10 FPS) sin tocar el WiFi.

## Seguridad

- TLS con `setInsecure()` (no valida el certificado) y token en NVS sin cifrar.
  Suficiente para uso doméstico.
- Pendiente (v2): pinear el certificado raíz de Anthropic y cifrar el token.

## Estructura

```
ClaudePet/
├── ClaudePet.ino         orquestación (setup/loop, comandos serie, cuenta atrás)
├── config.h              pines, defaults, umbrales, cabeceras
├── secrets_store.*       persistencia NVS (token / modelo / intervalo)
├── provisioning.*        portal cautivo WiFiManager
├── anthropic_client.*    sonda HTTPS + parseo de cabeceras 5h/7d/reset
├── pet.*                 máquina de estados de ánimo (con histéresis)
├── display.*             render del OLED (caras procedurales, barra, reloj)
└── docs/                 arquitectura, hardware, setup y desarrollo
```

Documentación completa: [Arquitectura](docs/ARCHITECTURE.md) ·
[Hardware](docs/HARDWARE.md) · [Puesta en marcha](docs/SETUP.md) ·
[Desarrollo](docs/DEVELOPMENT.md)

## Inspiración

- [oauramos/claude-usage-stick](https://github.com/oauramos/claude-usage-stick) — mismo enfoque autónomo.
- [dorofino/ClaudeGauge](https://github.com/dorofino/ClaudeGauge)
- [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy) — ideas de animaciones.
- [rootedlab-code/claude-code-usage-monitor](https://github.com/rootedlab-code/claude-code-usage-monitor) — variante con middleware.
