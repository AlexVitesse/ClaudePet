// provisioning.cpp — Implementación con WiFiManager + parámetros personalizados.
#include "provisioning.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "display.h"

namespace provisioning {

bool ensure(Settings& s) {
  // Cargamos lo que tengamos (token/modelo/intervalo).
  secrets::load(s);

  WiFiManager wm;
  wm.setConfigPortalTimeout(0);     // sin timeout: espera a que el usuario configure
  wm.setBreakAfterConfig(true);

  // Campos extra del portal.
  char model[48];
  char poll[8];
  snprintf(model, sizeof(model), "%s",
           s.model.length() ? s.model.c_str() : DEFAULT_MODEL);
  snprintf(poll, sizeof(poll), "%u", (unsigned)s.pollSeconds);

  WiFiManagerParameter pToken("token", "Token OAuth de Claude (Bearer)",
                              s.oauthToken.c_str(), 512);
  WiFiManagerParameter pModel("model", "Modelo (sonda)", model, 47);
  WiFiManagerParameter pPoll("poll", "Intervalo de sondeo (s)", poll, 7);
  wm.addParameter(&pToken);
  wm.addParameter(&pModel);
  wm.addParameter(&pPoll);

  bool needPortal = !secrets::isProvisioned();

  // Mensaje en el OLED mientras intenta/abre el portal.
  if (needPortal) {
    display::splash("Configura:", AP_NAME);
  } else {
    display::splash("Conectando", "WiFi...");
  }

  bool connected;
  if (needPortal) {
    // Fuerza el portal aunque haya WiFi guardado (falta el token).
    connected = wm.startConfigPortal(AP_NAME);
  } else {
    // Reusa WiFi guardado; abre portal solo si la conexión falla.
    connected = wm.autoConnect(AP_NAME);
  }

  // Si el usuario tocó el portal, persistimos los parámetros nuevos.
  String tok = pToken.getValue();
  String mdl = pModel.getValue();
  uint32_t pl = (uint32_t)String(pPoll.getValue()).toInt();
  if (tok.length()) s.oauthToken = tok;
  if (mdl.length()) s.model = mdl;
  if (pl >= MIN_POLL_SECONDS) s.pollSeconds = pl;
  if (s.pollSeconds < MIN_POLL_SECONDS) s.pollSeconds = DEFAULT_POLL_SECONDS;

  s.wifiSsid = WiFi.SSID();  // solo informativo
  secrets::save(s);

  return connected && s.oauthToken.length() > 0;
}

void factoryResetAndReboot() {
  WiFiManager wm;
  wm.resetSettings();   // borra credenciales WiFi guardadas por WiFiManager
  secrets::clear();     // borra token/modelo/intervalo
  delay(300);
  ESP.restart();
}

}  // namespace provisioning
