// anthropic_client.cpp — Implementación de la sonda HTTPS.
#include "anthropic_client.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"

namespace {
// Convierte un valor de cabecera de utilización a entero 0..100.
// Las cabeceras unified-*-utilization llegan como número (p.ej. "37" o "37.5").
int parseUtil(const String& v) {
  if (v.length() == 0) return -1;
  float f = v.toFloat();
  if (f < 0) f = 0;
  if (f > 100) f = 100;
  return (int)(f + 0.5f);
}
}  // namespace

namespace anthropic {

UsageResult probe(const String& oauthToken, const String& model) {
  UsageResult r;

  WiFiClientSecure client;
  // v1: sin validar el certificado para ahorrar RAM (uso doméstico).
  // Endurecimiento v2: client.setCACert(ANTHROPIC_ROOT_CA);
  client.setInsecure();
  client.setTimeout(15);  // segundos

  HTTPClient http;
  String url = String("https://") + ANTHROPIC_HOST + ANTHROPIC_PATH;
  if (!http.begin(client, url)) {
    r.httpCode = -1;
    return r;
  }

  http.addHeader("content-type", "application/json");
  http.addHeader("anthropic-version", ANTHROPIC_VERSION);
  http.addHeader("anthropic-beta", ANTHROPIC_OAUTH_BETA);
  http.addHeader("Authorization", String("Bearer ") + oauthToken);

  // Pedimos que se conserven las cabeceras de rate-limit para poder leerlas.
  const char* keep[] = {HDR_5H_UTIL, HDR_7D_UTIL, HDR_5H_RESET};
  http.collectHeaders(keep, sizeof(keep) / sizeof(keep[0]));

  // Body mínimo: 1 token de salida -> coste despreciable.
  String body = String("{\"model\":\"") + model +
                "\",\"max_tokens\":1,\"messages\":[{\"role\":\"user\",\"content\":\".\"}]}";

  int code = http.POST(body);
  r.httpCode = code;

  if (code == 401 || code == 403) {
    r.badToken = true;
    http.end();
    return r;
  }

  // Incluso un 429 trae las cabeceras de utilización; las leemos igual.
  if (code > 0) {
    r.used5h = parseUtil(http.header(HDR_5H_UTIL));
    r.used7d = parseUtil(http.header(HDR_7D_UTIL));
    String reset = http.header(HDR_5H_RESET);
    if (reset.length()) r.reset5h = reset.toInt();
    r.ok = (r.used5h >= 0);
  }

  http.end();
  return r;
}

}  // namespace anthropic
