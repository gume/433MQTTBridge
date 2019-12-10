#pragma once

#include <Arduino.h>
#define MAXTRANS 16

class OOKtranslate {
public:
  OOKtranslate(uint32_t _minST = 500, uint32_t _maxST = 10000, uint32_t _tickST = 200);

  void signal(uint32_t micros, bool value);
  void loop(uint32_t micros);
  
  void setCode(String signalString, String code);
  void setCodeCallback(void (*_userCodeCallback)(String code));
  void setUnknownCallback(void (*_userUnknownCallback)(String signalStr));
  
private:
  String checkCode(String signalString);

  uint32_t minST;	// Minimal signal hold time
  uint32_t maxST;	// Maximal signal hold time
  uint32_t tickST;	// Signal hold time
  
  String signalStr;
  uint32_t signalLen;
  char lastSignal;
  uint32_t lastSignalTime;
  char currentSignal;
  uint32_t currentSignalTime;

  //etl::map<String, String> codes;
  String codes[MAXTRANS];
  String signals[MAXTRANS];
  uint8_t transLen;
  
  void (*userCodeCallback)(String code);
  void (*userUnknownCallback)(String signalStr);
};
