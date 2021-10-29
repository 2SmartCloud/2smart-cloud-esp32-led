#pragma once

#include <map>
#include <string>
#include <vector>

extern std::vector<const char *> lsModes;
extern bool newLsMqttData;
extern bool needReboot;

bool sendLsSettings();
bool sendLsParams();
bool subscribeLsMqtt();
void sendLsParam(bool notifyEnabled);
bool handleLSMessage(char *topic, uint8_t *payload, unsigned int length);
