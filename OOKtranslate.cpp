#include "OOKtranslate.h"
#include <ArduinoJson.h>

OOKtranslate::OOKtranslate(uint32_t _minST, uint32_t _maxST) {
  minST = _minST;
  maxST = _maxST;
  
  signall = 0;
  
  userCodeCallback = NULL;
  userUnknownCallback = NULL;
  transLen = 0;
}

void OOKtranslate::signal(uint32_t t, bool value) {

  if (working) oops = true;

  if (signall == 0 && value == 0) {
    // Should not start with zero!
    return;
  }
  
  if (signall > 0) {
    if (signalv[signall-1] == value) {
      // Strange, but the same signal (missed something)
      return;  
    }
    signald[signall-1] = t - signald[signall-1];
  }
    
  signald[signall] = t;
  signalv[signall] = value;
  signall++;
  if (signall == MAXREC) {
    checkSignal();
  }
}

void OOKtranslate::checkSignal() {

  working = true;
  oops = false;
  
  String raw1 = "";
  String raw2 = "";
  uint32_t len1 = 0;
  uint32_t len2 = 0;
  
  if (userRawCallback) {
    for (int i = 0; i < signall; i++) {
      raw1 += String((char)(signalv[i] + '0'));
      raw1 += "[" + String(signald[i]) + "]";
      len1 += signald[i];
    }    
  }
  
  // Remove short signals
  int i = 0;
  int j = 0;
  while (i < signall) {
    if ((j > 0) && (signald[i] < minST)) {
      signald[j-1] += signald[i];
      i++;
    }
    else if ((j > 0) && (signalv[i] == signalv[j-1])) {
      signald[j-1] += signald[i];
      i++;      
    }
    else {
      signald[j] = signald[i];
      signalv[j] = signalv[i];
      i++; j++;
    }
  }
  signall = j;

  if (userRawCallback) {
    for (int i = 0; i < signall; i++) {
      raw2 += String((char)(signalv[i] + '0'));
      raw2 += "[" + String(signald[i]) + "]";
      len2 += signald[i];
    }    

    raw1 += "S[" + String(len1) + "]";
    raw2 += "S[" + String(len2) + "]";
    userRawCallback(raw1, raw2);
  }

  String signalStr = "/";
  for (int i = 0; i < signall; i += 2) {
    uint32_t hi = signald[i];
    uint32_t lo = signald[i+1];
    if (lo > bucketTimes[4]) {
      // Last signal in the row
      if (abs(hi - bucketTimes[0]) < abs(hi-bucketTimes[2])) signalStr += "0/";
      else signalStr += "1/";
    }
    else if (  (hi + lo)*10 > (bucketTimes[0] + bucketTimes[1])*12 &&
          (hi + lo)*10 > (bucketTimes[2] + bucketTimes[3])*12) {
      // Missing signals
      signalStr += "?";
      signald[i] = signald[i] - (bucketTimes[0] + bucketTimes[1] + bucketTimes[2] + bucketTimes[3])/2;
      i = i - 2;
    }
    else {
      uint32_t d1 = abs(hi-bucketTimes[0]) + abs(lo-bucketTimes[1]);
      uint32_t d2 = abs(hi-bucketTimes[2]) + abs(lo-bucketTimes[3]);
      if (d1 < d2) signalStr += "0";
      else signalStr += "1";
    }
  }

  if (signall > 1) {
    //Serial.println("\nDEBUG: " + signalStr);
    String code = checkCode(signalStr);
    if (code != "") {
      if (userCodeCallback) userCodeCallback(code);
    }
    else {
      if (userUnknownCallback) userUnknownCallback(signalStr);
    }
  }

  signall = 0;
  working = false;
  if (oops) Serial.println("OOPS!");
}

void OOKtranslate::loop(uint32_t t) {
  if (signall == 0) return;
  if (t - signald[signall-1] > maxST) {
    signald[signall-1] = maxST;
    checkSignal();  
  }
}

String OOKtranslate::checkCode(String signalString) {
  for (int i = 0; i < transLen; i++) {
    String c = "/" + signals[i] + "/";
    if (signalString.indexOf(c) >= 0) return codes[i];
  }
  return "";
}

void OOKtranslate::setCode(String signalString, String code) {
  if (transLen < MAXTRANS-1) {
    codes[transLen] = code;
    signals[transLen] = signalString;
    transLen++;
  }
}

void OOKtranslate::setCodeCallback(void (*_userCodeCallback)(String code)) {
  userCodeCallback = _userCodeCallback;
}

void OOKtranslate::setUnknownCallback(void (*_userUnknownCallback)(String signalStr)) {
  userUnknownCallback = _userUnknownCallback;
}

void OOKtranslate::setRawCallback(void (*_userRawCallback)(String raw1, String raw2)) {
  userRawCallback = _userRawCallback;
}
