#pragma once
#include <Arduino.h>

// ----------------------------------------------------------HTTP-----------
String ssid_name = "Wifi_Name";  // WiFi name
String ssid_password = "";       // WiFi password
String person_mail = "";
String person_id = "";
String token = "";
String host = "cloud.2smart.com";
String broker_port = "1883";
String web_auth_password = "";
const char* http_username = "admin";
// -------------------------------------------------------Production settings
String device_id = "";                   // DeviceID/ MAC:adress
String product_id = "1625478058454605";  // production id
// -------------------------------------------------------MQTT variables

const char* device_name = "2Smart Led Strip";
const char* device_version = "5";
const char* firmware_name = "1625478058454605";

void HandleMessage(char* topic, byte* payload, unsigned int length);
