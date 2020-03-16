#include "OOKtranslate.h"
#include <ArduinoJson.h>

OOKtranslate::OOKtranslate(uint32_t _minST, uint32_t _maxST) {
  minST = _minST;
  maxST = _maxST;
  
  signall = 0;
  
  userCodeCallback = NULL;
  userUnknownCallback = NULL;
  userRawCallback = NULL;
  
  transLen = 0;
}

// Incoming signal, time in us
void OOKtranslate::signal(uint32_t t, bool value) {

  if (signall == 0 && value == 0) {
    // Should not start with zero!
    return;
  }
  
  if (signall > 0) {
    if (signalv[signall-1] == value) {
      // Strange, but is is the same signal as previously. (might have missed something)
      return;  
    }
    signald[signall-1] = t - signald[signall-1];
  }
    
  signald[signall] = t;
  signalv[signall] = value;
  signall++;
  if (signall == MAXREC) {
    signall--;  // The last signal has no duration, so skip it
    // Try to tanslate the signal
    checkSignal();
  }
}

void OOKtranslate::checkSignal() {

  //if (userRawCallback) {
  //  userRawCallback(signalv, signald, signall);
  //}
  
  signall = removeNoise(minST, signalv, signald, signall);    // Overwrite the arrays!
  
  if (userRawCallback) {
    userRawCallback(signalv, signald, signall);
  }

  String signalStr = checkBuckets(bucketTimes, signalv, signald, signall);
  
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

void OOKtranslate::setRawCallback(void (*_userRawCallback)(uint8_t signalv[], uint32_t signald[], int signall)) {
  userRawCallback = _userRawCallback;
}


String OOKtranslate::signalToString(uint8_t sv[], uint32_t sd[], int sl) {
  String ss;
  for (int i = 0; i < sl; i++) {
    ss += String(sv[i]);
    ss += "[" + String(sd[i]) + "]";
  }
  return ss;
}

int OOKtranslate::removeNoise(int minLength, uint8_t sv[], uint32_t sd[], int sl) {
  // Remove short signals, which must be noise
  // Overwrite the arrays !!!
  int i = 0;
  int j = 0;
  while (i < sl) {
    if ((j > 0) && (sd[i] < minLength)) {
      sd[j-1] += sd[i];
      i++;
    }
    else if ((j > 0) && (sv[i] == sv[j-1])) {
      sd[j-1] += sd[i];
      i++;      
    }
    else {
      sd[j] = sd[i];
      sv[j] = sv[i];
      i++; j++;
    }
  }
  return j;
}

String OOKtranslate::checkBuckets(uint16_t bucketTimes[], uint8_t sv[], uint32_t sd[], int sl ) {

  String signalStr = "/";
  for (int i = 0; i < sl; i += 2) {
    uint32_t hi = sd[i];   // It should start with '1' !
    uint32_t lo = sd[i+1];
    
    // Test if the last tag is too long
    if (lo > bucketTimes[4]) {
      // Last signal in the row
      if (abs(hi - bucketTimes[0]) < abs(hi-bucketTimes[2])) signalStr += "0/";
      else signalStr += "1/";
    }
    // Test if the pair is too long
    else if (  (hi + lo)*10 > (bucketTimes[0] + bucketTimes[1])*12 &&
          (hi + lo)*10 > (bucketTimes[2] + bucketTimes[3])*12) {
      // Missing signals if the pair length is more than 20% of known signals
      uint16_t al = (bucketTimes[0] + bucketTimes[1] + bucketTimes[2] + bucketTimes[3]) / 2;
      int missing = (hi + lo + al/2) / al;
      for (int i = 0; i < missing; i++) signalStr += "*";
    }
    else {
    // Select the one which is closer to any of the buckets
      uint32_t d1 = abs(hi-bucketTimes[0]) + abs(lo-bucketTimes[1]);
      uint32_t d2 = abs(hi-bucketTimes[2]) + abs(lo-bucketTimes[3]);
      if (d1 < d2) signalStr += "0";
      else signalStr += "1";
    }
  }
  return signalStr;
}
