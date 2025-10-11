#include "tvbg_sender.h"

#ifndef TVBG_POST_SEND_GAP_MS
#define TVBG_POST_SEND_GAP_MS 35
#endif
#ifndef TVBG_NEC_REPEATS
#define TVBG_NEC_REPEATS 0
#endif
#if __has_include(<IRremoteESP8266.h>)
  #include <IRremoteESP8266.h>
  #include <IRsend.h>
  #define USE_IRREMOTE 1
  static IRsend irsend(IR_TX_PIN);
  static uint16_t sCachedKhz = 0;
#else
  #include "driver/ledc.h"
  #define USE_IRREMOTE 0
  static const ledc_channel_t CH = LEDC_CHANNEL_0;
  static const ledc_timer_t   TM = LEDC_TIMER_0;
  static const int LEDC_RES_BITS = 10; 
  static uint16_t sCachedKhz = 0;
  static inline void carrierOn(uint16_t khz){
    uint32_t freq = (uint32_t)khz * 1000;
    ledcSetup(CH, freq, LEDC_RES_BITS);
    ledcAttachPin(IR_TX_PIN, CH);
    ledcWrite(CH, (1 << (LEDC_RES_BITS-1))); 
  }
  static inline void carrierOff(){
    ledcWrite(CH, 0);
    ledcDetachPin(IR_TX_PIN);
  }
  static inline void delayMicrosAccurate(uint32_t us){
    
    uint32_t start = micros();
    if (us > 15000) { delayMicroseconds(us); return; }
    while ((micros() - start) < us) { }
  }
  static void sendRaw38k(const uint16_t* seq, uint16_t len, uint16_t khz){
    for (uint16_t i=0;i<len;i++){
      if ((i & 1) == 0) { 
        carrierOn(khz);
        delayMicrosAccurate(seq[i]);
      } else {            
        carrierOff();
        delayMicrosAccurate(seq[i]);
      }
    }
    carrierOff();
  }
#endif

void tvbgSenderBegin(){
#if USE_IRREMOTE
  irsend.begin();
  sCachedKhz = 0;
#else
  ledcSetup(CH, 38000, LEDC_RES_BITS);
  pinMode(IR_TX_PIN, OUTPUT);
  digitalWrite(IR_TX_PIN, LOW);
  sCachedKhz = 0;
#endif
  delay(TVBG_POST_SEND_GAP_MS);
}

void tvbgSend(const TVBGToken& tok){
  uint16_t targetKhz = tok.khz ? tok.khz : 38;
  
#if USE_IRREMOTE
  if (sCachedKhz != targetKhz) {
    irsend.enableIROut(targetKhz);
    sCachedKhz = targetKhz;
  }
  
  switch (tok.proto){
    case IRProto::NEC:       
      irsend.sendNEC(tok.value, tok.nbits, TVBG_NEC_REPEATS); 
      break;
    case IRProto::SONY:      
      irsend.sendSony(tok.value, tok.nbits, 2); 
      break; 
    case IRProto::PANASONIC: 
      irsend.sendPanasonic64(tok.value); 
      break;
    case IRProto::RC5:       
      irsend.sendRC5(tok.value, tok.nbits); 
      break;
    case IRProto::RC6:       
      irsend.sendRC6(tok.value, tok.nbits); 
      break;
    case IRProto::RAW:
    default:
      if (tok.raw && tok.rawLen) {
        irsend.sendRaw(tok.raw, tok.rawLen, targetKhz);
      }
      break;
  }
#else
  if (tok.proto == IRProto::RAW && tok.raw && tok.rawLen){
    sendRaw38k(tok.raw, tok.rawLen, targetKhz);
  } else {
    if (sCachedKhz != targetKhz) {
      carrierOn(targetKhz);
      sCachedKhz = targetKhz;
    }
    delayMicrosAccurate(600);
    carrierOff();
  }
#endif
  delay(TVBG_POST_SEND_GAP_MS); 
}

void tvbgSenderEnd(){
#if !USE_IRREMOTE
  carrierOff();
#endif
}
