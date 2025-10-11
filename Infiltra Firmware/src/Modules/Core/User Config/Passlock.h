#pragma once
#include <Arduino.h>
namespace Core {
enum class PasslockType : uint8_t {
  None = 0,
  Numeric4 = 1,
  Pattern  = 2
};

struct Passlock {
  static void begin();
  static bool isEnabled();
  static PasslockType type();
  static bool promptForUnlock();
  static bool runSettingsFlow();
};
} 
