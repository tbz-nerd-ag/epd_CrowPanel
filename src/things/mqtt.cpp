#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <header.h>
#include <HTTPUpdate.h>

#include "epd/EPD.h"
#include "epd/EPD_GUI.h"

#define DOORSIGN_HEIGHT 300
#define DOORSIGN_WIDTH 400

uint8_t* imageBuffer = nullptr;
const int IMAGE_SIZE = 400 * 300 / 8;  // = 15000 Bytes

char server[32];
uint16_t port;
uint16_t id;
uint16_t errorcode;

bool noerrorimage1;

void callback(char* topic, byte* payload, unsigned int length);

bool loadMQTTData() {
    if (!prefs.begin("mqtt", true)) {
        return false;
    }
    
    if (!prefs.isKey("server") || !prefs.isKey("port") || !prefs.isKey("number")) {
        prefs.end();
        return false;
    }

    //laed wlan daten aus den nvs
    prefs.begin("mqtt", true);
    prefs.getString("server", server, sizeof(server));
    port = prefs.getUShort("port", 1883);
    id = prefs.getUShort("number");
    prefs.end();

    prefs.begin("errorimage", true);
    noerrorimage1 = prefs.getBool("noerrorimage", false);
    prefs.end();

    prefs.begin("error-code", false);
    prefs.getUShort("error",errorcode);
    prefs.end();

    return true;
}

void connectMQTT() {
    if (!loadMQTTData()) {
        if (noerrorimage1) {
            esp_sleep_enable_timer_wakeup(10*60*1000000ULL); //10Min
            esp_deep_sleep_start();
        } else {
            MQTTErrorPicture();
            return;
        }
    }

    mqtt.setServer(server, port);
    mqtt.setCallback(callback);
    mqtt.setBufferSize(16000);
    mqtt.setKeepAlive(60);
    
    char clientId[8];
    char topic[32];
    char image[32];
    char sleep[32];
    char awakePayload[32];

    snprintf(clientId, sizeof(clientId), "%u", id); 
    snprintf(topic,    sizeof(topic),    "%u/awake", id);
    snprintf(image,    sizeof(image),    "%u/image", id);
    snprintf(sleep,    sizeof(sleep),    "%u/sleep", id);

    float batt = readBatteryVoltage();

    int versuche = 40;
    while (!mqtt.connected() && versuche-- > 0) {
        String strid = String(id);
        if (mqtt.connect(clientId)) {
            snprintf(awakePayload, sizeof(awakePayload), "awake,%d,%d", batt, errorcode);

            mqtt.publish(topic, awakePayload);

            //mqtt.publish(topic, "awake,100,0x00");
            mqtt.subscribe(image);
            mqtt.subscribe(sleep);
        } else {
            if (noerrorimage1) {
                esp_sleep_enable_timer_wakeup(10*60*1000000ULL); //10Min
                esp_deep_sleep_start();
            } else {
                MQTTErrorPicture();
            }
        }
    }
    

}

void callback(char* topic, byte* payload, unsigned int length) {
    imageBuffer = (uint8_t*)ps_malloc(IMAGE_SIZE);

    //topic definition
    char imageTopic[32];
    char sleepTopic[32];
    char otaupdateTopic[32];

    snprintf(imageTopic, sizeof(imageTopic), "%u/image", id);
    snprintf(sleepTopic, sizeof(sleepTopic), "%u/sleep", id);
    snprintf(otaupdateTopic, sizeof(otaupdateTopic), "%u/otaupdate", id);

    if (strcmp(topic, imageTopic) == 0) {
        if (length == IMAGE_SIZE) {
            memcpy(imageBuffer, payload, length);
      
            Serial.println("Bild empfangen, zeige an...");
            EPD_Init();
            EPD_Clear();
            EPD_Display(imageBuffer);
            EPD_Sleep();
        } else {
        Serial.printf("Falsche Größe: %d Bytes (erwartet: %d)\n", length, IMAGE_SIZE);
        }
    } else if (strcmp(topic, sleepTopic) == 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.*s", length, (char*)payload);

        int seconds = 0;
        char flag[8] = {0};

        sscanf(buf, "%d,%7s", &seconds, flag);
        
        bool noerrorimage = (strcmp(flag, "true") == 0);

        prefs.begin("errorimage", false);
        prefs.putBool("noerrorimage", noerrorimage);
        prefs.end();

        char gnTopic[32];
        char gnPayload[32];
        snprintf(gnTopic,   sizeof(gnTopic),   "%u/gn", id);
        snprintf(gnPayload, sizeof(gnPayload), "goodnight,%d", seconds);
        mqtt.publish(gnTopic, gnPayload);

        delay(500);

        prefs.begin("error-code", false);
        prefs.putUShort("error",0x00);
        prefs.end();

        esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
        esp_deep_sleep_start(); 
    } else if (strcmp(topic, otaupdateTopic) == 0) {
        String url = "";
        for (int i = 0; i < length; i++) url += (char)payload[i];

        t_httpUpdate_return ret = httpUpdate.update(espClient, url);

        switch (ret)
        {
        case HTTP_UPDATE_OK:
            break;
        case HTTP_UPDATE_FAILED:
            OTAErrorPicture();
            break;
        }
    }
}