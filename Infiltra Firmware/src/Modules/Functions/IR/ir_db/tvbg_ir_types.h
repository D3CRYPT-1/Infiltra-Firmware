#pragma once
#include <Arduino.h>

enum class IRProto : uint8_t { RAW=0, NEC, SONY, PANASONIC, RC5, RC6 };
struct TVBGToken {
  const char*  brand;   
  const char*  label;   
  IRProto      proto;   
  uint32_t     value;   
  uint16_t     nbits;   
  const uint16_t* raw;  
  uint16_t     rawLen;  
  uint16_t     khz;     
};
