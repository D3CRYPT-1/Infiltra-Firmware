#pragma once
#include <Arduino.h>
#include "ir_db/tvbg_ir_types.h"

#ifndef IR_TX_PIN
#define IR_TX_PIN 9
#endif

void tvbgSenderBegin();     
void tvbgSend(const TVBGToken& tok); 
void tvbgSenderEnd();
