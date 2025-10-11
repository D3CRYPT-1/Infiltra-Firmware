#pragma once
#include "tvbg_ir_types.h"

static const TVBGToken TVBG_VOL_UP_LIST[] PROGMEM = {
  { "LG",      "Vol+",  IRProto::NEC,  0x20DF40BF, 32, nullptr, 0, 38 },
  { "Samsung", "Vol+",  IRProto::NEC,  0xE0E0E01F, 32, nullptr, 0, 38 },
  { "Sony",    "Vol+",  IRProto::SONY, 0x048,      12, nullptr, 0, 40 }, 
};
static constexpr size_t TVBG_VOL_UP_COUNT = sizeof(TVBG_VOL_UP_LIST)/sizeof(TVBG_VOL_UP_LIST[0]);

static const TVBGToken TVBG_VOL_DOWN_LIST[] PROGMEM = {
  { "LG",      "Vol-",  IRProto::NEC,  0x20DFC03F, 32, nullptr, 0, 38 },
  { "Samsung", "Vol-",  IRProto::NEC,  0xE0E0D02F, 32, nullptr, 0, 38 },
  { "Sony",    "Vol-",  IRProto::SONY, 0x049,      12, nullptr, 0, 40 }, 
};
static constexpr size_t TVBG_VOL_DOWN_COUNT = sizeof(TVBG_VOL_DOWN_LIST)/sizeof(TVBG_VOL_DOWN_LIST[0]);
