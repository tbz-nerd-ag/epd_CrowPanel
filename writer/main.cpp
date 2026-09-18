#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

Preferences prefs;

//String ssid = "";
//String password = "";
//String mqttserver = "192.168.103.200";

String ssid = "";
String password = "";
String mqttserver = "192.168.133.50";

uint16_t mqttport = 1883;
uint16_t id = 0x0A;
uint16_t errorcode = 0x00;

void setup() {
    nvs_flash_erase();
    nvs_flash_init(); 

    prefs.begin("wlan", false);
    prefs.putString("ssid", ssid);
    prefs.putString("password", password);
    prefs.end();

    prefs.begin("mqtt", false);
    prefs.putString("server", mqttserver);
    prefs.putUShort("port", mqttport);
    prefs.putUShort("number", id);
    prefs.end();

    prefs.begin("error-code", false);
    prefs.putUShort("error",errorcode);
    prefs.end();

}

void loop() {
}