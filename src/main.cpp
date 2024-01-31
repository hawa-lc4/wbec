// Copyright (c) 2021 steff393, MIT license

#include <Arduino.h>
// #include <ArduinoOTA.h>
// #include <button.h>
#include <globalConfig.h>
#include <goEmulator.h>
// #include <inverter.h>
#include <LittleFS.h>
#include <loadManager.h>
#include <logger.h>
#include <mbComm.h>
#include <mqtt.h>
#include <phaseCtrl.h>
// #include <powerfox.h>
#include <pvAlgo.h>
// #include <rfid.h>
// #include <shelly.h>
#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>
#include <webServer.h>
#include <webSocket.h>


void setup() {
#if WALLE_VERSION_MAJOR == 1
  Serial1.begin(115200);
  Serial1.println(F("\n\nStarting wall-E ;-)"));
#endif
#if WALLE_VERSION_MAJOR == 2
  Serial.begin(115200);
  Serial.println(F("\n\nStarting wall-E ;-)"));
  pinMode(PIN_PHASECTRL, OUTPUT);
  digitalWrite(PIN_PHASECTRL, LOW);					// Relais OFF
#endif
  logger_allocate();
  
  if(!LittleFS.begin()){ 
#if WALLE_VERSION_MAJOR == 1
    Serial1.println(F("An Error has occurred while mounting LittleFS"));
#endif
#if WALLE_VERSION_MAJOR == 2
    Serial.println(F("An Error has occurred while mounting LittleFS"));
#endif
  return;
  }

  loadConfig();

  WiFiManager wifiManager;
  char ssid[16]; strcpy(ssid, cfgApSsid);
  char pass[32]; strcpy(pass, cfgApPass);
  // wifiManager.setDebugOutput(true);
  wifiManager.setConnectTimeout(cfgWifiConnectTimeout);
  wifiManager.autoConnect(ssid, pass);

  // still experimental (see #12):
  if (cfgWifiSleepMode >= WIFI_NONE_SLEEP && cfgWifiSleepMode <= WIFI_MODEM_SLEEP) {
    WiFi.setSleepMode((WiFiSleepType_t)cfgWifiSleepMode);
  }

  logger_setup();

  // setup the Webserver
  webServer_setup();
  webSocket_setup();

  // setup the OTA server
  // ArduinoOTA.setHostname("wbec");
  // ArduinoOTA.begin();

  // ArduinoOTA.onStart([]() 
  // {
  //   _handlingOTA = true;
  // });

  mb_setup();
  mqtt_begin();
  // rfid_setup();
  // powerfox_setup();
  // shelly_setup();
  // inverter_setup();
  // btn_setup();
  pv_setup();
  lm_setup();
#if WALLE_VERSION_MAJOR == 1
  Serial1.print(F("Boot time: ")); Serial1.println(millis());
  Serial1.print(F("Free heap: ")); Serial1.println(ESP.getFreeHeap());
#endif
#if WALLE_VERSION_MAJOR == 2
  Serial.print(F("Boot time: ")); Serial.println(millis());
  Serial.print(F("Free heap: ")); Serial.println(ESP.getFreeHeap());
#endif
}


void loop() {
  // ArduinoOTA.handle();
  // if(!_handlingOTA) {
    logger_loop();
    mb_loop();
    goE_handle();
    mqtt_handle();
    webServer_loop();
    webSocket_loop();
    // rfid_loop();
    // powerfox_loop(); 
    // shelly_loop();
    // inverter_loop();
    // btn_loop();
    pv_loop();
    pc_handle();
    lm_loop();
    if (cfgLoopDelay <= 10) {          // see #18, might have an effect to reactivity of webserver in some environments 
      delay(cfgLoopDelay);
    }
  // }
}
