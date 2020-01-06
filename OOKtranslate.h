#pragma once

#include <Arduino.h>
#define MAXTRANS 16
#define MAXREC 500

class OOKtranslate {
public:
  OOKtranslate(uint32_t _minST = 100, uint32_t _maxST = 10000);

  void signal(uint32_t micros, bool value);
  void loop(uint32_t micros);
  
  void setCode(String signalString, String code);
  void setCodeCallback(void (*_userCodeCallback)(String code));
  void setUnknownCallback(void (*_userUnknownCallback)(String signalStr));
  void setRawCallback(void (*_userRawCallback)(String raw1, String raw2));
  
private:
  void checkSignal();
  String checkCode(String signalString);

  uint32_t minST;	// Minimal signal hold time
  uint32_t maxST;	// Maximal signal hold time
  
  uint32_t signald[MAXREC];   // Duration of signals
  uint8_t signalv[MAXREC];    // Signal value
  uint8_t signall;            // Length of the signal

  //etl::map<String, String> codes;
  String codes[MAXTRANS];
  String signals[MAXTRANS];
  uint8_t transLen;
  
  void (*userCodeCallback)(String code);
  void (*userUnknownCallback)(String signalStr);
  void (*userRawCallback) (String raw1, String raw2);

  uint16_t bucketTimes[6] = { 320, 640, 800, 160, 6000 };
};
