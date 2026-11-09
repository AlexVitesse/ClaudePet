# Arquitectura de ClaudePet

## Objetivo
Mostrar en una mascota física de escritorio cuánta **cuota de Claude (plan
Pro/Max)** queda en la **ventana de 5 horas**, con expresiones de ánimo
(feliz → pánico) en una OLED 0.96" 128×64.

## Topología: ESP32 autónomo
El ESP32 habla **directo** con la API de Anthropic por HTTPS. No hay PC ni
middleware: el dispositivo queda enchufado y funciona solo.

```
┌──────────────────────────┐       HTTPS (cada ~120 s)        ┌──────────────────┐
│  ESP32-C3 + OLED SSD1306 │ ───────────────────────────────▶ │ api.anthropic.com │
│                          │   POST /v1/messages (max_tokens:1)│  /v1/messages    │
│                          │ ◀─────────────────────────────── │                  │
│   lee cabeceras:         │     cabeceras de rate-limit       └──────────────────┘
│   unified-5h-utilization │
│   unified-7d-utilization │
└────────────┬─────────────┘
             │ restante = 100 - utilización
             ▼
   máquina de estados (pet) ──▶ cara + barra en el OLED
```

## Fuente de datos
Se hace una request **mínima** (`max_tokens: 1`, coste despreciable) y se leen
las cabeceras de la respuesta:

| Cabecera | Significado |
|----------|-------------|
| `anthropic-ratelimit-unified-5h-utilization` | % usado de la ventana de 5 h |
| `anthropic-ratelimit-unified-7d-utilization` | % usado de la ventana de 7 días |
| `anthropic-ratelimit-unified-5h-reset` | epoch Unix del reset; se resta la cabecera `Date` para la cuenta atrás |

La autenticación es con un **token OAuth** de la suscripción (cabecera
`Authorization: Bearer ...` + `anthropic-beta: oauth-2025-04-20`). **No** se usa
una API key de pago.

> **Insight clave:** ya no hace falta scraping con Playwright/Selenium para leer
> el límite de Claude Pro: estas cabeceras exponen la utilización de la
> suscripción directamente.

## Flujo de ejecución
1. **Boot** → inicializa OLED → carga config de NVS.
2. Si no hay token/WiFi → abre **portal cautivo** (`ClaudePet-setup`).
3. Conecta al WiFi.
4. **Loop**:
   - Cada `pollSeconds` (default 120 s): sonda la API, lee utilización 5h,
     calcula `restante = 100 - usado`, actualiza el estado.
   - Siempre (~10 FPS): anima la cara según el estado (sin tocar la red).

## Mapeo cuota → ánimo
Basado en **% restante** de la ventana de 5 h, con **histéresis** de ±3% para no
rebotar entre estados en los umbrales.

| % restante | Estado |
|-----------:|--------|
| 80–100% | Feliz |
| 40–79%  | Neutral |
| 11–39%  | Ansioso |
| 0–10%   | Pánico |
| —       | Sin WiFi / Token inválido (estados de error) |

## Principios de bajo consumo
- **Sondeo poco frecuente** (default 120 s ⇒ ~30 req/h, triviales). La ventana de
  5 h cambia lento; no tiene sentido sondear más rápido.
- La animación corre **local**; entre sondeos no se usa WiFi.
- OLED con contraste reducido.
- TLS con `setInsecure()` en v1 para minimizar RAM (ver
  [SECURITY](#seguridad)).

## Seguridad
- **v1 (actual):** TLS sin validar certificado (`setInsecure()`) para ahorrar
  RAM; token guardado en NVS en claro. Aceptable para uso doméstico en red
  propia.
- **v2 (endurecimiento):** pinear el certificado raíz de Anthropic
  (`setCACert`) y cifrar el token con AES-256-GCM derivado de un PIN.

## Decisiones de diseño
| Decisión | Alternativa descartada | Motivo |
|----------|------------------------|--------|
| ESP32 autónomo | Middleware Python en PC | No depender de una PC encendida |
| Ventana 5h Pro/Max (OAuth) | API key de pago / scraping web | Sin coste extra, sin navegador |
| Caras procedurales (U8g2) | Sprites XBM | Más ligero y fácil de tunear |
| HTTP pull cada 120s | MQTT push | Sin infraestructura; basta para esta señal |

## Referencias
- [oauramos/claude-usage-stick](https://github.com/oauramos/claude-usage-stick) — mismo enfoque autónomo.
- [anthropics/claude-desktop-buddy](https://github.com/anthropics/claude-desktop-buddy) — mascota oficial (animaciones).
- [dorofino/ClaudeGauge](https://github.com/dorofino/ClaudeGauge)
- [rootedlab-code/claude-code-usage-monitor](https://github.com/rootedlab-code/claude-code-usage-monitor) — variante middleware (JSONL).
