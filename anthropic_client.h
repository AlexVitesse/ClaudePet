// anthropic_client.h — Sonda HTTPS a la API de Anthropic para leer la
// utilización de la ventana de 5h/7d desde las cabeceras de respuesta.
#pragma once
#include <Arduino.h>

struct UsageResult {
  bool ok = false;          // true si se obtuvieron cabeceras válidas
  bool badToken = false;    // 401: token inválido/caducado
  int used5h = -1;          // % usado de la ventana de 5h (0..100)
  int used7d = -1;          // % usado de la ventana de 7d (0..100)
  long reset5h = -1;        // segundos hasta el reset de la ventana de 5h (si viene)
  int httpCode = 0;         // código HTTP devuelto (diagnóstico)
};

namespace anthropic {
// Hace una request mínima (max_tokens:1) con el token OAuth y devuelve la
// utilización leída de las cabeceras. No bloquea más allá del timeout HTTP.
UsageResult probe(const String& oauthToken, const String& model);
}  // namespace anthropic
