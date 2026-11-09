// main.cpp — Orquestación de ClaudePet.
//   setup(): OLED -> provisión/WiFi -> primer sondeo.
//   loop():  anima la mascota (local, ~10 FPS) y sondea la API cada N segundos.
//
// Bajo consumo: la red solo se usa en el sondeo (default 120 s). Entre sondeos
// solo corre la animación, que no toca WiFi.
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "secrets_store.h"
#include "provisioning.h"
#include "display.h"
#include "anthropic_client.h"
#include "pet.h"

static Settings g_settings;
static PetState g_state = PetState::Neutral;
static int g_remaining = -1;        // % restante de la ventana de 5h
static int g_overrideRemaining = -1; // para pruebas por serial (-1 = off)
static uint32_t g_frame = 0;

static long     g_reset5h = -1;        // segundos hasta el reset (snapshot del último poll)
static uint32_t g_resetAnchorMs = 0;   // millis() en el instante de ese snapshot

static const uint32_t ANIM_INTERVAL_MS = 100;  // ~10 FPS
static uint32_t g_lastAnim = 0;
static uint32_t g_lastPoll = 0;
static bool g_firstPoll = true;

// Reconexión WiFi sin bloquear.
static uint32_t g_lastWifiTry = 0;

void doPoll() {
  if (WiFi.status() != WL_CONNECTED) {
    g_state = PetState::Disconnected;
    g_remaining = -1;
    return;
  }

  UsageResult u = anthropic::probe(g_settings.oauthToken, g_settings.model);
  if (u.badToken) {
    g_state = PetState::BadToken;
    g_remaining = -1;
    Serial.println("[poll] 401/403 token invalido");
    return;
  }
  if (!u.ok) {
    Serial.printf("[poll] sin cabeceras (http=%d)\n", u.httpCode);
    // mantenemos último estado conocido si lo había; si no, neutral
    return;
  }

  g_remaining = 100 - u.used5h;
  if (u.reset5h >= 0) {
    g_reset5h = u.reset5h;
    g_resetAnchorMs = millis();
  }
  g_state = pet::stateFromRemaining(g_remaining, g_state);
  Serial.printf("[poll] used5h=%d%% used7d=%d%% -> restante=%d%% reset=%lds estado=%s\n",
                u.used5h, u.used7d, g_remaining, u.reset5h, pet::name(g_state));
}

// Comandos de prueba por serial:
//   r<NN>     fija % restante simulado (p.ej. "r5")  | "r-" desactiva override
//   p         fuerza un sondeo ahora
//   t<TOKEN>  reemplaza el token OAuth y lo guarda (sin tocar el WiFi)
//   x         factory reset
void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  char c = line[0];
  if (c == 'r') {
    if (line.length() >= 2 && line[1] == '-') {
      g_overrideRemaining = -1;
      Serial.println("[test] override OFF");
    } else {
      g_overrideRemaining = constrain(line.substring(1).toInt(), 0, 100);
      Serial.printf("[test] override restante=%d%%\n", g_overrideRemaining);
    }
  } else if (c == 'p') {
    Serial.println("[test] sondeo forzado");
    doPoll();
    g_lastPoll = millis();
  } else if (c == 't') {
    String tok = line.substring(1);
    tok.trim();
    if (tok.length() > 20) {
      g_settings.oauthToken = tok;
      secrets::save(g_settings);
      Serial.println("[test] token actualizado y guardado; sondeando...");
      g_state = PetState::Neutral;
      doPoll();
      g_lastPoll = millis();
    } else {
      Serial.println("[test] token demasiado corto, ignorado");
    }
  } else if (c == 'x') {
    Serial.println("[test] factory reset...");
    provisioning::factoryResetAndReboot();
  }
}

// Rellena `out` con la cuenta atrás al reset de la ventana de 5h.
// Devuelve false si no hay dato (oculta el texto).
static bool formatResetCountdown(char* out, size_t n, uint32_t now) {
  if (g_reset5h < 0) return false;
  long elapsed = (long)((now - g_resetAnchorMs) / 1000UL);
  long left = g_reset5h - elapsed;
  if (left < 0) left = 0;                       // ya debería haberse reseteado
  long h = left / 3600;
  long m = (left % 3600) / 60;
  if (h > 0)      snprintf(out, n, "%ldh%02ldm", h, m);
  else if (m > 0) snprintf(out, n, "%ldm", m);
  else            snprintf(out, n, "<1m");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nClaudePet booting...");
  display::begin();
  display::splash("ClaudePet", "iniciando...");

  // Botón opcional para forzar portal al arrancar.
#if BUTTON_PIN >= 0
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(50);
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Boton pulsado: factory reset");
    provisioning::factoryResetAndReboot();
  }
#endif

  if (!provisioning::ensure(g_settings)) {
    Serial.println("Provisioning fallo o requiere portal, reiniciando...");
    display::splash("Reiniciando", "config...");
    delay(1500);
    ESP.restart();
  }

  Serial.printf("WiFi OK: %s  poll=%us  model=%s\n",
                WiFi.SSID().c_str(), (unsigned)g_settings.pollSeconds,
                g_settings.model.c_str());
  display::splash("Conectado!", WiFi.SSID().c_str());
  delay(800);
}

void loop() {
  uint32_t now = millis();
  handleSerial();

  // --- Sondeo periódico (o el primero inmediato) ---
  if (g_firstPoll || (now - g_lastPoll) >= g_settings.pollSeconds * 1000UL) {
    doPoll();
    g_lastPoll = now;
    g_firstPoll = false;
  }

  // --- Reintento de WiFi si se cayó (cada 10 s, no bloqueante) ---
  if (WiFi.status() != WL_CONNECTED && (now - g_lastWifiTry) > 10000) {
    g_lastWifiTry = now;
    WiFi.reconnect();
  }

  // --- Animación local ---
  if (now - g_lastAnim >= ANIM_INTERVAL_MS) {
    g_lastAnim = now;
    g_frame++;

    PetState s = g_state;
    int rem = g_remaining;
    if (g_overrideRemaining >= 0) {       // modo prueba por serial
      rem = g_overrideRemaining;
      s = pet::stateFromRemaining(rem, s);
    }
    char resetBuf[12];
    bool hasReset = formatResetCountdown(resetBuf, sizeof(resetBuf), now);
    display::render(s, rem, g_frame, hasReset ? resetBuf : nullptr);
  }
}
