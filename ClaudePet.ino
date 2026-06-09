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
  g_state = pet::stateFromRemaining(g_remaining, g_state);
  Serial.printf("[poll] used5h=%d%% used7d=%d%% -> restante=%d%% estado=%s\n",
                u.used5h, u.used7d, g_remaining, pet::name(g_state));
}

// Comandos de prueba por serial:
//   r<NN>  fija % restante simulado (p.ej. "r5")  | "r-" desactiva override
//   p      fuerza un sondeo ahora
//   x      factory reset
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
  } else if (c == 'x') {
    Serial.println("[test] factory reset...");
    provisioning::factoryResetAndReboot();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
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
    display::render(s, rem, g_frame);
  }
}
