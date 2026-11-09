// config.h — Parámetros de compilación de ClaudePet.
// Los datos sensibles (WiFi, token OAuth) NO viven aquí: se introducen por el
// portal cautivo y se guardan en NVS (ver secrets_store / provisioning).
#pragma once

// ---------------------------------------------------------------------------
// Pines del OLED SSD1306 128x64 I2C
// ESP32 clásico (DevKit): SDA=21, SCL=22  |  ESP32-C3 SuperMini: SDA=8, SCL=9.
// ---------------------------------------------------------------------------
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

// Botón opcional (long-press = borrar config / abrir portal). -1 = sin botón.
#define BUTTON_PIN -1  // GPIO libre con pull-up (ojo: en ESP32 clásico GPIO9 es de la flash)
// Nota: si BUTTON_PIN coincide con un pin I2C, déjalo en -1 para evitar choques.
#undef BUTTON_PIN
#define BUTTON_PIN -1

// ---------------------------------------------------------------------------
// API de Anthropic
// ---------------------------------------------------------------------------
#define ANTHROPIC_HOST     "api.anthropic.com"
#define ANTHROPIC_PATH     "/v1/messages"
#define ANTHROPIC_VERSION  "2023-06-01"
#define ANTHROPIC_OAUTH_BETA "oauth-2025-04-20"
// Sin un User-Agent tipo Claude Code, las peticiones OAuth caen en un bucket
// muy limitado (429 persistentes). Ajustable si cambia el formato esperado.
#define HTTP_USER_AGENT    "claude-cli/1.0.0 (external, cli)"
// Modelo barato para la sonda; max_tokens:1 hace la request casi gratis.
#define DEFAULT_MODEL      "claude-haiku-4-5-20251001"

// Cabeceras de rate-limit que leemos de la respuesta.
#define HDR_5H_UTIL  "anthropic-ratelimit-unified-5h-utilization"
#define HDR_7D_UTIL  "anthropic-ratelimit-unified-7d-utilization"
#define HDR_5H_RESET "anthropic-ratelimit-unified-5h-reset"

// ---------------------------------------------------------------------------
// Comportamiento / consumo
// ---------------------------------------------------------------------------
#define DEFAULT_POLL_SECONDS 120   // sondeo a la API (mín. recomendado 60s)
#define MIN_POLL_SECONDS      30
#define WIFI_CONNECT_TIMEOUT_MS 20000
#define AP_NAME "ClaudePet-setup"  // SSID del portal cautivo

// Umbrales de ánimo por % RESTANTE de la ventana de 5h.
#define TH_HAPPY   80   // >= 80% restante -> Feliz
#define TH_NEUTRAL 40   // >= 40% restante -> Neutral
#define TH_ANXIOUS 11   // >= 11% restante -> Ansioso
// < 11% restante -> Pánico
#define STATE_HYSTERESIS 3  // banda muerta (%) para no rebotar entre estados
