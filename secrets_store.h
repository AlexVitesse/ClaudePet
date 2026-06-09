// secrets_store.h — Persistencia en NVS de WiFi, token OAuth, modelo e intervalo.
#pragma once
#include <Arduino.h>

struct Settings {
  String wifiSsid;
  String wifiPass;
  String oauthToken;
  String model;
  uint32_t pollSeconds;
};

namespace secrets {
// Carga la configuración guardada. Devuelve false si no hay token/WiFi (primer
// arranque sin provisión).
bool load(Settings& out);

// Guarda la configuración completa.
void save(const Settings& s);

// Borra toda la configuración (factory reset).
void clear();

// True si existe configuración mínima (WiFi + token) para arrancar.
bool isProvisioned();
}  // namespace secrets
