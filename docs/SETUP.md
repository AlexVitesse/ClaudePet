# Puesta en marcha (Arduino IDE)

## 1. Soporte para placas ESP32
1. `Archivo > Preferencias > Gestor de URLs adicionales de tarjetas`, añade:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. `Herramientas > Placa > Gestor de tarjetas`, busca **esp32** (Espressif) e
   instala.

## 2. Librerías (Gestor de librerías)
- **U8g2** (oliver / olikraus)
- **WiFiManager** (tzapu)

## 3. Abrir y subir
1. Abre `ClaudePet.ino`. **Importante:** todos los `.cpp/.h` deben estar en la
   misma carpeta, que debe llamarse exactamente `ClaudePet`.
2. `Herramientas > Placa`: selecciona tu ESP32-C3 (p. ej. *ESP32C3 Dev Module*
   o *LOLIN C3 Mini*).
3. En placas C3, activa **USB CDC On Boot: Enabled** para ver el Serial por USB.
4. Selecciona el **Puerto** y pulsa **Subir** (→).
5. Abre el **Monitor Serie** a **115200** baudios.

## 4. Configuración por portal cautivo
En el **primer arranque** (o tras factory reset) el ESP32 crea la red WiFi
`ClaudePet-setup`:
1. Conéctate a esa red desde el móvil/PC.
2. Abre `http://192.168.4.1`.
3. Elige tu WiFi, pega tu **Token OAuth** (ver abajo) y, si quieres, ajusta el
   **modelo** y el **intervalo de sondeo** (default 120 s).
4. Guarda. El ESP32 se reinicia y empieza a mostrar la cuota.

## 5. Obtener el Token OAuth de Claude
Con [Claude Code](https://docs.anthropic.com/en/docs/claude-code) y tu cuenta
Pro/Max, genera un token de larga duración:
```
claude setup-token
```
Copia el token (`sk-ant-oat...`) y pégalo en el portal. **No uses** el
`accessToken` de `~/.claude/.credentials.json`: Claude Code lo rota y caduca en
minutos.

> ⚠️ Trata el token como una contraseña: da acceso a tu cuenta de Claude. Nunca
> lo subas a git (el `.gitignore` ya bloquea `.credentials.json` y `*.token`).
> Si caduca, la mascota muestra el estado `TOKEN?`; regenéralo y reconfigura por
> el portal.

## 6. Pruebas por Monitor Serie
Con el Monitor Serie abierto (115200, fin de línea "Nueva línea"):

| Comando | Efecto |
|---------|--------|
| `r5`    | Simula 5% restante (Pánico) |
| `r60`   | Simula 60% (Neutral) |
| `r-`    | Desactiva la simulación |
| `p`     | Fuerza un sondeo real ahora |
| `t<TOKEN>` | Reemplaza el token sin pasar por el portal |
| `x`     | Factory reset (borra WiFi y token) |

En cada sondeo verás:
```
[poll] used5h=NN% used7d=NN% -> restante=NN% reset=NNNNs estado=...
```

## Troubleshooting
| Síntoma | Causa probable | Solución |
|---------|----------------|----------|
| OLED en blanco | Cableado/pines I2C | Revisa SDA/SCL y `config.h` |
| No aparece `ClaudePet-setup` | Ya estaba provisionado | Manda `x` por serial o usa el botón |
| Estado `TOKEN?` | Token caducado/incorrecto | `claude setup-token` y mándalo con `t<TOKEN>` |
| Estado `SIN WIFI` | WiFi fuera de alcance | Acerca el dispositivo / reconfigura WiFi |
| No se ve el Serial (C3) | USB CDC desactivado | Activa *USB CDC On Boot: Enabled* |
