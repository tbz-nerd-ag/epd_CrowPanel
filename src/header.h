#pragma once

#include <Preferences.h>
extern Preferences prefs;

#include <PubSubClient.h>
#include <WiFi.h>
extern WiFiClient espClient;
extern PubSubClient mqtt;

//wifi.cpp
void connectWifi(void);
bool loadWifiData();

//errorImage.cpp
void WifiErrorPicture(void);
void MQTTErrorPicture(void);
void OTAErrorPicture(void);

//mqtt.cpp
void connectMQTT(void);
bool loadMQTTData();

//battery.cpp
float readBatteryVoltage(void);
void    initBatteryADC(void);