#include "main.h"

#include <string>

#include "autoUpd.h"
#include "customWifi.h"
#include "fileSystem.h"
#include "gpio.h"
#include "ledStrip.h"
#include "mqtt.h"
#include "notifications.h"
#include "ntpTime.h"
#include "webServer.h"

void setup() {
    Serial.begin(115200);
    setGpios();
    if (!initFiles() || !loadConfig()) {
        delay(5000);
        ESP.restart();
    }
    initLedstrip();
    if (!loadFwSettings()) {
        fwSettings.version = deviceVersion.toInt();
        newFwSettings = true;
    }
    if (deviceId.length() <= 1) {
        mac = WiFi.macAddress();
        mac.toLowerCase();
        mac.replace(":", "-");
        deviceId = mac;
    }
    uint32_t last_blink_AP = millis();
    if (ssidName == "Wifi_Name" || ssidName == "") {
        startWifiAp();
        setupWebServer();
    }
    while (ssidName == "Wifi_Name" || ssidName == "") {
        if (millis() - last_blink_AP > BLINKING_TIME_AP_MODE) {
            Serial.print("/");
            digitalWrite(LED_STATUS, !digitalRead(LED_STATUS));
            last_blink_AP = millis();
        }
        checkButtons();
        checkNewState();
    }
    while (!wifiInit()) {
        checkButtons();
        checkNewState();
        if (eraseFlag) {
            eraseFlash();
        }
    }
    initNtp();
    setupWebServer();
    mac = WiFi.macAddress();
    Serial.println(mac);
    Serial.print("IP: ");
    localIp = WiFi.localIP().toString();
    Serial.println(localIp);
    if (initMqtt(handleMessage)) {  // MQTT initialization
        initNodes();
    }
}

void loop() {
    if (needReboot) {
        ESP.restart();
    }

    mqttLoop();
    if (millis() - lastMillisHeartbit > HEARTBIT_DELAY * 1000) {
        if (wifiInit()) {
            sendHeartbit();
            lastMillisHeartbit = millis();
        }
    }
    if (reconnectMqtt && (millis() - reconnectMqttTime > DELAY_FOR_RECOONECT_MQTT)) {
        reconnect();
    }
    if (eraseFlag) {
        eraseFlash();
    }
    checkButtons();
    checkNewState();
    if (newLsMqttData) {
        Serial.printf("new state - %s\r\n", lsModes.at(ls.mode));

        sendLsParam(notifySettings.system);
    }
    checkFirmware(deviceVersion.toInt(), notifySettings.update);

    if (mqttReconnected && initNodes()) mqttReconnected = false;
}

bool initNodes() {
    return sendLsSettings() && sendLsParams() && subscribeLsMqtt() && sendFwSettings() && sendFwParams() &&
           subscribeFwMqtt() && initNotify();

    // If there are a lot of devices, then when the broker restarts, there will be many republish of their topics
}

void handleMessage(char *topic, byte *payload, unsigned int length) {
    uint32_t time = millis();

    if (strstr(topic, "ledstrip")) {
        if (handleLSMessage(topic, payload, length)) {
            Serial.printf("handle time %lu\r\n", millis() - time);
            return;
        }
    }
    if (strstr(topic, "firmware")) {
        if (handleFwMessage(topic, payload, length)) {
            Serial.printf("handle time %lu\r\n", millis() - time);
            return;
        }
    }
    if (strstr(topic, "notifications")) {
        if (handleNotifyMessage(topic, payload, length)) {
            Serial.printf("handle time %lu\r\n", millis() - time);
            return;
        }
    }

    Serial.printf("handle time %lu\r\n", millis() - time);
}
