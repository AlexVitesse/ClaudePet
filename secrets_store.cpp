// secrets_store.cpp — Implementación NVS con la librería Preferences.
#include "secrets_store.h"
#include <Preferences.h>
#include "config.h"

static const char* NS = "claudepet";
static Preferences prefs;

namespace secrets {

bool load(Settings& out) {
  prefs.begin(NS, /*readOnly=*/true);
  out.wifiSsid   = prefs.getString("ssid", "");
  out.wifiPass   = prefs.getString("pass", "");
  out.oauthToken = prefs.getString("token", "");
  out.model      = prefs.getString("model", DEFAULT_MODEL);
  out.pollSeconds = prefs.getUInt("poll", DEFAULT_POLL_SECONDS);
  prefs.end();

  if (out.pollSeconds < MIN_POLL_SECONDS) out.pollSeconds = MIN_POLL_SECONDS;
  // WiFiManager gestiona las credenciales WiFi; aquí el marcador de provisión es
  // el token OAuth.
  return out.oauthToken.length() > 0;
}

void save(const Settings& s) {
  prefs.begin(NS, /*readOnly=*/false);
  prefs.putString("ssid", s.wifiSsid);
  prefs.putString("pass", s.wifiPass);
  prefs.putString("token", s.oauthToken);
  prefs.putString("model", s.model.length() ? s.model : String(DEFAULT_MODEL));
  prefs.putUInt("poll", s.pollSeconds < MIN_POLL_SECONDS ? MIN_POLL_SECONDS
                                                         : s.pollSeconds);
  prefs.end();
}

void clear() {
  prefs.begin(NS, /*readOnly=*/false);
  prefs.clear();
  prefs.end();
}

bool isProvisioned() {
  prefs.begin(NS, /*readOnly=*/true);
  bool ok = prefs.getString("token", "").length() > 0;
  prefs.end();
  return ok;
}

}  // namespace secrets
