#pragma once
#include <Arduino.h>

#include "ledStripMqtt.h"

#define LS_OFF_TIME 2000

enum LsNewState { NO_CHANGES, NEW_COLOR, NEW_BRIGHTNESS, NEW_MODE };
enum LedStripStates { RAINBOW, COLOR, DISCO };  // must be the same with lsModes

typedef struct {
    bool state;
    uint8_t mode;
    uint8_t brightness;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint16_t quantity;
} LsSettings;

extern LsSettings ls;

extern uint8_t newLsState;
extern char rgbValues[12];
extern uint8_t rgbLength;

extern bool stateChanged;

void initLedstrip();
void checkNewState();
bool loadLsSettings();
bool saveLsSettings();
// --------------------------- modes
void turnOffLs();
void extractColor();
void changeColor();
void rainbow();
void disco();
