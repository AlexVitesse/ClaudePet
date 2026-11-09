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

// Convierte una cabecera HTTP Date en formato IMF-fixdate (RFC 7231), p.ej.
// "Sun, 22 Jun 2026 14:11:15 GMT", a epoch Unix (UTC). Devuelve -1 si falla.
// Nos da la hora "ahora" del servidor sin necesitar NTP en el dispositivo.
long parseHttpDate(const String& d) {
  if (d.length() < 25) return -1;
  int comma = d.indexOf(',');
  if (comma < 0) return -1;
  int i = comma + 1;
  while (i < (int)d.length() && d[i] == ' ') i++;
  if (i + 20 > (int)d.length()) return -1;

  int day  = d.substring(i,      i + 2 ).toInt();
  String mon = d.substring(i + 3, i + 6);
  int year = d.substring(i + 7,  i + 11).toInt();
  int hh   = d.substring(i + 12, i + 14).toInt();
  int mm   = d.substring(i + 15, i + 17).toInt();
  int ss   = d.substring(i + 18, i + 20).toInt();

  static const char* M = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int mi = -1;
  for (int k = 0; k < 12; k++) {
    if (mon.equals(String(M).substring(k * 3, k * 3 + 3))) { mi = k + 1; break; }
  }
  if (mi < 1 || day < 1 || year < 1970) return -1;

  // days_from_civil (algoritmo de Howard Hinnant): fecha UTC -> días desde epoch.
  long y = year - (mi <= 2 ? 1 : 0);
  long era = (y >= 0 ? y : y - 399) / 400;
  long yoe = y - era * 400;
  long doy = (153L * (mi + (mi > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = era * 146097 + doe - 719468;
  return days * 86400L + hh * 3600L + mm * 60L + ss;
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

  http.setUserAgent(HTTP_USER_AGENT);  // evita el bucket de rate-limit agresivo
  http.addHeader("content-type", "application/json");
  http.addHeader("anthropic-version", ANTHROPIC_VERSION);
  http.addHeader("anthropic-beta", ANTHROPIC_OAUTH_BETA);
  http.addHeader("Authorization", String("Bearer ") + oauthToken);

  // Pedimos que se conserven las cabeceras de rate-limit (y "Date" para
  // convertir el epoch de reset en cuenta atrás sin NTP).
  const char* keep[] = {HDR_5H_UTIL, HDR_7D_UTIL, HDR_5H_RESET, "Date"};
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

    // La cabecera de reset llega como epoch Unix absoluto (no "segundos
    // restantes"). La pasamos a cuenta atrás restando la hora del servidor,
    // que tomamos de la cabecera "Date" de esta misma respuesta.
    long resetVal = http.header(HDR_5H_RESET).toInt();
    if (resetVal > 1000000000L) {            // parece epoch absoluto
      long nowEpoch = parseHttpDate(http.header("Date"));
      if (nowEpoch > 0) {
        long left = resetVal - nowEpoch;
        r.reset5h = (left < 0) ? 0 : left;   // segundos hasta el reset
      }                                       // sin "Date" -> -1 (se oculta)
    } else if (resetVal > 0) {
      r.reset5h = resetVal;                   // ya venía como segundos restantes
    }
    r.ok = (r.used5h >= 0);
  }

  http.end();
  return r;
}

}  // namespace anthropic
