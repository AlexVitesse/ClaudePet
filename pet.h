// pet.h — Máquina de estados de ánimo de la mascota.
#pragma once
#include <Arduino.h>

enum class PetState : uint8_t {
  Happy,        // saciado
  Neutral,
  Anxious,
  Panic,        // cuota agotada
  Disconnected, // sin WiFi
  BadToken      // 401: token inválido/caducado
};

namespace pet {
// Devuelve el estado según el % RESTANTE (0..100) de la ventana de 5h, aplicando
// histéresis respecto al estado previo para evitar rebotes en los umbrales.
PetState stateFromRemaining(int remainingPct, PetState previous);

const char* name(PetState s);
}  // namespace pet
