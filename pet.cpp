// pet.cpp — Mapeo cuota -> ánimo con histéresis.
#include "pet.h"
#include "config.h"

namespace pet {

PetState stateFromRemaining(int rem, PetState prev) {
  if (rem < 0) rem = 0;
  if (rem > 100) rem = 100;

  // Umbrales base.
  int thHappy = TH_HAPPY;
  int thNeutral = TH_NEUTRAL;
  int thAnxious = TH_ANXIOUS;

  // Histéresis: si ya estábamos en un estado "mejor", exige caer un poco más
  // para degradar; si estábamos "peor", exige subir un poco más para mejorar.
  const int H = STATE_HYSTERESIS;
  switch (prev) {
    case PetState::Happy:   thHappy   -= H; break;
    case PetState::Neutral: thNeutral -= H; thHappy += H; break;
    case PetState::Anxious: thAnxious -= H; thNeutral += H; break;
    case PetState::Panic:   thAnxious += H; break;
    default: break;  // estados de error no aplican histéresis de cuota
  }

  if (rem >= thHappy)   return PetState::Happy;
  if (rem >= thNeutral) return PetState::Neutral;
  if (rem >= thAnxious) return PetState::Anxious;
  return PetState::Panic;
}

const char* name(PetState s) {
  switch (s) {
    case PetState::Happy:        return "FELIZ";
    case PetState::Neutral:      return "NEUTRAL";
    case PetState::Anxious:      return "ANSIOSO";
    case PetState::Panic:        return "PANICO";
    case PetState::Disconnected: return "SIN WIFI";
    case PetState::BadToken:     return "TOKEN?";
  }
  return "?";
}

}  // namespace pet
