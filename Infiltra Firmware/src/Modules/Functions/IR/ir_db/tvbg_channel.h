#pragma once
#include "tvbg_ir_types.h"

static const TVBGToken TVBG_CH_UP_LIST[] PROGMEM = {
  { "LG",      "Ch+", IRProto::NEC,  0x20DF00FF, 32, nullptr, 0, 38 },
  { "Samsung", "Ch+", IRProto::NEC,  0xE0E048B7, 32, nullptr, 0, 38 },
};
static constexpr size_t TVBG_CH_UP_COUNT = sizeof(TVBG_CH_UP_LIST)/sizeof(TVBG_CH_UP_LIST[0]);

static const TVBGToken TVBG_CH_DOWN_LIST[] PROGMEM = {
  { "LG",      "Ch-", IRProto::NEC,  0x20DF807F, 32, nullptr, 0, 38 },
  { "Samsung", "Ch-", IRProto::NEC,  0xE0E008F7, 32, nullptr, 0, 38 },
};
static constexpr size_t TVBG_CH_DOWN_COUNT = sizeof(TVBG_CH_DOWN_LIST)/sizeof(TVBG_CH_DOWN_LIST[0]);
