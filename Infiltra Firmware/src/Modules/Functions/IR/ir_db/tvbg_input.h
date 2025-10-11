#pragma once
#include "tvbg_ir_types.h"

static const TVBGToken TVBG_INPUT_LIST[] PROGMEM = {
  { "LG",      "Input", IRProto::NEC,  0x20DFD02F, 32, nullptr, 0, 38 },
  { "Samsung", "Input", IRProto::NEC,  0xE0E0D629, 32, nullptr, 0, 38 },
};
static constexpr size_t TVBG_INPUT_COUNT = sizeof(TVBG_INPUT_LIST)/sizeof(TVBG_INPUT_LIST[0]);
