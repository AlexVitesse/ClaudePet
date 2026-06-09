// provisioning.h — Portal cautivo (WiFiManager) para capturar WiFi + token
// OAuth + modelo + intervalo de sondeo.
#pragma once
#include <Arduino.h>
#include "secrets_store.h"

namespace provisioning {
// Conecta al WiFi guardado; si no hay WiFi o no hay token, abre el portal
// cautivo (AP "ClaudePet-setup" en 192.168.4.1). Bloquea hasta que haya WiFi y
// token. Rellena `s` con la configuración final (ya persistida en NVS).
// Devuelve true si quedó conectado y provisionado.
bool ensure(Settings& s);

// Borra config (WiFi de WiFiManager + NVS propia) y reinicia.
void factoryResetAndReboot();
}  // namespace provisioning
