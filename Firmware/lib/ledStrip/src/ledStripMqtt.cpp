#include "ledStripMqtt.h"

#include "ledStrip.h"
#include "mqtt.h"

//  ---------------------------------------- ls - mean led Strip

const uint16_t MAX_LED_QUANTITY = 1800;
bool newLsMqttData = false;

std::vector<const char *> lsModes{"rainbow", "color", "disco"};  // must be the same with LedStripStates

enum topics { T_SWITCH, T_COLOR, T_BRIGHTNESS, T_MODE, T_QUNATITY };  // T-topic

std::map<uint8_t, std::string> subsLsTopics = {{T_SWITCH, "ledstrip/switch/set"},
                                               {T_COLOR, "ledstrip/color/set"},
                                               {T_BRIGHTNESS, "ledstrip/brightness/set"},
                                               {T_MODE, "ledstrip/mode/set"},
                                               {T_QUNATITY, "ledstrip/quantity/set"}};

uint16_t getLsModesSize() { return lsModes.size(); }

bool sendLsSettings() {
    topicsWithValues topics = {{"ledstrip/$name", "ledStrip"},
                               {"ledstrip/$state", "ready"},
                               {"ledstrip/switch/$name", "switch"},  // switch on/off
                               {"ledstrip/switch/$settable", "true"},
                               {"ledstrip/switch/$retained", "true"},
                               {"ledstrip/switch/$datatype", "boolean"},
                               {"ledstrip/color/$name", "Color"},  // color
                               {"ledstrip/color/$settable", "true"},
                               {"ledstrip/color/$retained", "true"},
                               {"ledstrip/color/$datatype", "color"},
                               {"ledstrip/color/$format", "rgb"},
                               {"ledstrip/brightness/$name", "Brightness"},  // brightness
                               {"ledstrip/brightness/$settable", "true"},
                               {"ledstrip/brightness/$retained", "true"},
                               {"ledstrip/brightness/$datatype", "integer"},
                               {"ledstrip/brightness/$format", "0:100"},
                               {"ledstrip/mode/$name", "mode"},  // mode
                               {"ledstrip/mode/$settable", "true"},
                               {"ledstrip/mode/$retained", "true"},
                               {"ledstrip/mode/$datatype", "enum"},
                               {"ledstrip/mode/$unit", "#"},
                               {"ledstrip/quantity/$name", "NumLeds"},  // quantity
                               {"ledstrip/quantity/$settable", "true"},
                               {"ledstrip/quantity/$retained", "true"},
                               {"ledstrip/quantity/$datatype", "integer"}};

    return (mqttPublish(topics));
}

bool sendLsParams() {
    std::map<const char *, std::string> lsSettings = {{"ledstrip/brightness", ""}, {"ledstrip/color", ""},
                                                      {"ledstrip/mode", ""},       {"ledstrip/mode/$format", ""},
                                                      {"ledstrip/switch", ""},     {"ledstrip/quantity", ""}};

    char messageBuffer[12];  // length of RGB mess

    lsSettings["ledstrip/switch"] = (ls.state) ? "true" : "false";

    snprintf(messageBuffer, sizeof(messageBuffer), "%d,%d,%d", ls.red, ls.green, ls.blue);
    lsSettings["ledstrip/color"] = messageBuffer;

    snprintf(messageBuffer, sizeof(messageBuffer), "%d", ls.brightness);
    lsSettings["ledstrip/brightness"] = messageBuffer;

    lsSettings["ledstrip/mode"] = lsModes.at(ls.mode);

    std::string modeFormatValue = "";
    for (int i = 0; i < lsModes.size(); i++) {
        modeFormatValue += lsModes.at(i);
        if (lsModes.size() - i - 1) modeFormatValue += ",";
    }
    lsSettings["ledstrip/mode/$format"] = modeFormatValue;

    snprintf(messageBuffer, sizeof(messageBuffer), "%d", ls.quantity);
    lsSettings["ledstrip/quantity"] = messageBuffer;

    for (auto it = lsSettings.begin(); it != lsSettings.end(); ++it) {
        if (!mqttPublish(it->first, it->second.c_str(), true)) {
            return false;
        }
    }
    return (true);
}

bool subscribeLsMqtt() { return (mqttSubscribe(subsLsTopics)); }

void sendLsParam(bool notifyEnabled) {
    std::string topic = subsLsTopics.find(T_MODE)->second;
    mqttPublish(removeSlashSetSubtopic(topic), lsModes.at(ls.mode), true);

    topic = subsLsTopics.find(T_SWITCH)->second;
    mqttPublish(removeSlashSetSubtopic(topic), (ls.state) ? "true" : "false", true);

    char messageBuffer[12];  // length of RGB mess

    snprintf(messageBuffer, sizeof(messageBuffer), "%d,%d,%d", ls.red, ls.green, ls.blue);

    topic = subsLsTopics.find(T_COLOR)->second;
    mqttPublish(removeSlashSetSubtopic(topic), messageBuffer, true);

    snprintf(messageBuffer, sizeof(messageBuffer), "%d", ls.brightness);
    topic = subsLsTopics.find(T_BRIGHTNESS)->second;
    mqttPublish(removeSlashSetSubtopic(topic), messageBuffer, true);

    if (notifyEnabled && stateChanged && ls.state) sendNotification("Led on");
    if (notifyEnabled && !ls.state) sendNotification("Led off");
    stateChanged = false;
    newLsMqttData = false;
}

bool handleLSMessage(char *topic, byte *payload, unsigned int length) {
    String response;
    for (int i = 0; i < length; i++) {
        response += static_cast<char>(payload[i]);  // ------------ write payload
    }
    String topicHolder = topic;

    bool messHandled = false;

    std::string buffSuffix = subsLsTopics.find(T_SWITCH)->second;

    if (topicHolder.endsWith(buffSuffix.c_str())) {  // switch
        newLsState = NEW_MODE;
        ls.state = (static_cast<char>(payload[0]) == 't') ? true : false;

        mqttPublish(removeSlashSetSubtopic(buffSuffix), (ls.state) ? "true" : "false", true);

        messHandled = true;
    }

    buffSuffix = subsLsTopics.find(T_COLOR)->second;

    if (topicHolder.endsWith(buffSuffix.c_str())) {  // color
        newLsState = NEW_COLOR;
        rgbLength = length;
        for (int i = 0; i < length; i++) {
            rgbValues[i] = (static_cast<char>(payload[i]));
        }

        mqttPublish(removeSlashSetSubtopic(buffSuffix), response.c_str(), true);

        messHandled = true;
    }

    buffSuffix = subsLsTopics.find(T_BRIGHTNESS)->second;

    if (topicHolder.endsWith(buffSuffix.c_str())) {  // brightness
        newLsState = NEW_BRIGHTNESS;
        ls.brightness = response.toInt();

        mqttPublish(removeSlashSetSubtopic(buffSuffix), response.c_str(), true);

        messHandled = true;
    }

    buffSuffix = subsLsTopics.find(T_MODE)->second;

    if (topicHolder.endsWith(buffSuffix.c_str())) {  // mode
        newLsState = NEW_MODE;
        for (int i = 0; i < lsModes.size(); i++) {
            if (response == lsModes.at(i)) {
                ls.mode = i;
            }
        }
        ls.state = true;

        mqttPublish(removeSlashSetSubtopic(buffSuffix), response.c_str(), true);

        buffSuffix = subsLsTopics.find(T_SWITCH)->second;
        mqttPublish(removeSlashSetSubtopic(buffSuffix), "true", true);

        messHandled = true;
    }

    buffSuffix = subsLsTopics.find(T_QUNATITY)->second;

    if (topicHolder.endsWith(buffSuffix.c_str())) {  // quantity
        uint16_t newQuantity = response.toInt();
        if (newQuantity < MAX_LED_QUANTITY) {
            ls.quantity = newQuantity;
            mqttPublish(removeSlashSetSubtopic(buffSuffix), response.c_str(), true);
            turnOffLs();
            if (saveLsSettings()) {
                turnOffLs();
                needReboot = true;
            }
        } else {
            response = ls.quantity;
            mqttPublish(removeSlashSetSubtopic(buffSuffix), response.c_str(), true);
        }

        messHandled = true;
    }
    return messHandled;
}
