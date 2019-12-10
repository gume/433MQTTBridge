//
// Bridge 433 remote control to WiFi MQTT
// RFM6900K has to be modified to run on ESP8266
// The RFM69OOK::isr0 definition should start with 'ICACHE_RAM_ATTR'
//

#include <RFM69OOK.h>
#include <SPI.h>
#include <RFM69OOKregisters.h>
#include "OOKtranslate.h"
#include <IotWebConf.h>
#include <MQTT.h>
#include <ArduinoJson.h>

void wifiConnected();
void mqttMessageReceived(String &topic, String &payload);
void remotePressed(String code);

const char thingName[] = "433MQTTBridge";
const char wifiInitialApPassword[] = "password";
#define CONFIG_VERSION "4mb1"
#define CONFIG_PIN D2
#define STATUS_PIN LED_BUILTIN
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);

#define STRING_LEN 128
char mqttServerValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];
char hassDiscoveryValue[STRING_LEN];
IotWebConfParameter mqttServerParam = IotWebConfParameter("MQTT server", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfParameter mqttUserNameParam = IotWebConfParameter("MQTT user", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfParameter mqttUserPasswordParam = IotWebConfParameter("MQTT password", "mqttPass", mqttUserPasswordValue, STRING_LEN, "password");
IotWebConfParameter hassDiscoveryParam = IotWebConfParameter("HASS discovery topic", "hassDiscovery", hassDiscoveryValue, STRING_LEN, "homeassistant");

RFM69OOK radio(D4, D8, true, digitalPinToInterrupt(D8));
OOKtranslate ot;
bool rstate = false;  // For polling

volatile byte interrupt;
void myInterrupt() {
  interrupt = 1;
}


void handleRoot() {
  if (iotWebConf.handleCaptivePortal()) return;
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>433 to MQTT Bridge</title></head><body>Bridge is running!<p>";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";
  server.send(200, "text/html", s);
}

MQTTClient mqttClient(512);
WiFiClient net;
boolean needMqttConnect = false;
uint32_t lastMqttConnectionAttempt = 0;



void setup() {
  Serial.begin(74880);

  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);  
  iotWebConf.addParameter(&mqttServerParam);
  iotWebConf.addParameter(&mqttUserNameParam);
  iotWebConf.addParameter(&mqttUserPasswordParam);
  iotWebConf.addParameter(&hassDiscoveryParam);
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    mqttServerValue[0] = '\0';
    mqttUserNameValue[0] = '\0';
    mqttUserPasswordValue[0] = '\0';
    String("homeassistant").toCharArray(hassDiscoveryValue, STRING_LEN);
  }
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  mqttClient.begin(mqttServerValue, net);
  mqttClient.onMessage(mqttMessageReceived);

  radio.initialize();
  radio.setFixedThreshold(30);
  radio.setFrequencyMHz(433.714);
  // Interrupt is not working with WiFi :(
  //radio.receiveBegin();
  //radio.attachUserInterrupt(myInterrupt);
  // Do polling for 433 data
  pinMode(D8, INPUT);
  // Perform protected setMode(RF69OOK_MODE_RX) manually
  radio.writeReg(REG_OPMODE, (radio.readReg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
  radio.writeReg(REG_TESTPA1, 0x55);
  radio.writeReg(REG_TESTPA2, 0x70);

  ot.setCode("1[31]0[8]1[5]0[8]1[5]0[8]1[5]0[8]1[18]0[8]", "A+");
  ot.setCode("1[31]0[8]1[5]0[8]1[5]0[8]1[5]0[8]1[5]0[8]", "A-");
  ot.setCode("1[18]0[8]1[18]0[8]1[5]0[8]1[5]0[8]1[18]0[8]", "B+");
  ot.setCode("1[18]0[8]1[18]0[8]1[5]0[8]1[5]0[8]1[5]0[8]", "B-");
  ot.setCode("1[18]0[8]1[5]0[8]1[18]0[8]1[5]0[8]1[18]0[8]", "C+");
  ot.setCode("1[18]0[8]1[5]0[8]1[18]0[8]1[5]0[8]1[5]0[8]", "C-");
  ot.setCode("1[18]0[8]1[5]0[8]1[5]0[8]1[18]0[8]1[18]0[8]", "D+");
  ot.setCode("1[18]0[8]1[5]0[8]1[5]0[8]1[18]0[8]1[5]0[8]", "D-");
  //ot.setCode("1[18]0[8]1[5]0[8]1[5]0[8]1[5]0[8]1[5]0[34]", "NULL");

  ot.setCodeCallback(remotePressed);
  //ot.setUnknownCallback(remotePressed);

  Serial.println(F("start"));
}

void loop() {

  if (interrupt) {
    interrupt = 0;
    ot.signal(micros(), radio.poll());
  }  

  iotWebConf.doLoop();
  mqttClient.loop();
  ot.loop(micros());

  // Do polling instead of interrupts
  if (rstate != radio.poll()) {
    rstate = radio.poll();
    interrupt = 1;
  }

  if (needMqttConnect) {
    if (connectMqtt()) {
      needMqttConnect = false;
    }
  } else if ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && (!mqttClient.connected())) {
    Serial.println("MQTT reconnect");
    needMqttConnect = true;
  }
}

void wifiConnected()
{
  needMqttConnect = true;
}

boolean connectMqtt() {
  unsigned long now = millis();
  if (1000 > now - lastMqttConnectionAttempt) {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Connecting to MQTT server...");
  if (!connectMqttOptions()) {
    lastMqttConnectionAttempt = now;
    return false;
  }
  Serial.println("MQTT Connected!");
  
  // <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
  mqttClient.setWill(String(String(hassDiscoveryValue)+"/sensor/433mqttbridge/config").c_str());
  mqttClient.publish(
    String(hassDiscoveryValue)+"/sensor/433mqttbridge/config",
    "{\"name\": \"433mqttbridge\","
    " \"state_topic\": \"433mqttbridge/state\","
    " \"val_tpl\": \"{{value_json.code}}\"}"
    );
  Serial.println("MQTT discovery published.");
  
  mqttClient.subscribe(String(hassDiscoveryValue)+"/status");
  return true;
}

boolean connectMqttOptions() {
  boolean result;
  if (mqttUserPasswordValue[0] != '\0') {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue, mqttUserPasswordValue);
  }
  else if (mqttUserNameValue[0] != '\0') {
    result = mqttClient.connect(iotWebConf.getThingName(), mqttUserNameValue);
  }
  else {
    result = mqttClient.connect(iotWebConf.getThingName());
  }
  return result;
}

void mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);

  if (topic == String(hassDiscoveryValue)+"/status") {
    Serial.println("MQTT disconnect.");
    mqttClient.disconnect();
    // It will reconnect
    return;
  }

  StaticJsonDocument<200> msg;
  DeserializationError error = deserializeJson(msg, payload);
  if (error) return;

  /*
  if (msg.containsKey("level")) {
    int target = msg["level"];
    bike.setLevel(target);
    mqttClient.publish("/smartgymbike/debug", "level set to " + String(target));
  }
  */
}

void remotePressed(String code) {
  Serial.println(code);
  mqttClient.publish("433mqttbridge/state", "{\"code\": \""+code+"\"}");
}
