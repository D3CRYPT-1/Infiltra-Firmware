#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>

enum NFCState {
    NFC_IDLE,
    NFC_READING,
    NFC_SUCCESS,
    NFC_ERROR
};

enum NFCProtocol {
    NFC_PROTOCOL_UNKNOWN,
    NFC_PROTOCOL_ISO14443A,
    NFC_PROTOCOL_ISO14443B,
    NFC_PROTOCOL_ISO18092,  
    NFC_PROTOCOL_FELICA,
    NFC_PROTOCOL_MIFARE
};

struct NFCCardData {
    String uid;
    String type;
    String data;
    uint16_t dataSize;
    NFCProtocol protocol;
    uint8_t sak;  
    uint8_t atqa[2];  
    bool isValid;
};

void nfcReaderReset();
void nfcReaderDrawScreen(TFT_eSPI& tft);
void nfcReaderHandleInput(bool btnA, bool btnB, bool btnC, bool& exitReq);
void nfcReaderInit();
void nfcReaderUpdate();
NFCState nfcReaderGetState();
NFCCardData nfcReaderGetLastCard();