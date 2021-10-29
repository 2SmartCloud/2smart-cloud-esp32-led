#include "ledStrip.h"

#include <FastLED.h>

#include "fileSystem.h"

#define DATA_PIN 18
#define SAVE_STRIP_SETTINGS_TIME 10000
#define MAX_mA 1500

// --------------- Default setting if config file read failed
#define FIRST_ON_BRIGHTNESS 10
#define STATE RAINBOW
#define DEFAULT_COLOR 80
#define DEFAULT_LED_QUANTITY 300  // 5m led strip
//  ---------------

LsSettings ls = {1, 0, 0, 0, 0, 0, 1};

// ---------------MQTT Settings

uint8_t newLsState = 0;
uint8_t rgbLength;
uint32_t lastUpdateTime = 0;
char rgbValues[12];
bool newDataForSave = false;
bool stateChanged = false;

CRGB *ledsPtr;
uint8_t counter;

void checkNewState() {
    switch (newLsState) {
        case NO_CHANGES:
            break;
        case NEW_COLOR:
            extractColor();
            break;
        case NEW_BRIGHTNESS:
            FastLED.setBrightness(ls.brightness * 2);
            FastLED.show();
            break;
        case NEW_MODE:
            break;
    }
    if (ls.state) {
        switch (ls.mode) {
            case COLOR:
                if (newLsState == NO_CHANGES) break;
                changeColor();
                break;
            case RAINBOW:
                rainbow();
                break;
            case DISCO:
                disco();
                break;
        }
    } else if (newLsState) {
        turnOffLs();
    }
    if (newLsState != NO_CHANGES) {
        lastUpdateTime = millis();
        newLsState = NO_CHANGES;
        newDataForSave = true;
    }
    if (millis() - lastUpdateTime > SAVE_STRIP_SETTINGS_TIME && newDataForSave) {
        saveLsSettings();
        newDataForSave = false;
    }
}

void turnOffLs() {
    FastLED.clear();
    FastLED.show();
}

void initLedstrip() {
    if (!loadLsSettings()) {
        ls.brightness = FIRST_ON_BRIGHTNESS;
        ls.state = true;
        ls.mode = STATE;
        ls.red = DEFAULT_COLOR;
        ls.green = DEFAULT_COLOR * 2;
        ls.blue = DEFAULT_COLOR * 3;
        ls.quantity = DEFAULT_LED_QUANTITY;
    }
    ledsPtr = new CRGB[ls.quantity];
    FastLED.addLeds<WS2812, DATA_PIN, GRB>(ledsPtr, ls.quantity).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_mA);
    newLsState = NO_CHANGES;
    FastLED.setBrightness(ls.brightness * 2);
    if (ls.mode == COLOR) newLsState = NEW_MODE;
    checkNewState();
}

void changeColor() {
    for (int led = 0; led < ls.quantity; led++) {
        ledsPtr[led] = CRGB(ls.red, ls.green, ls.blue);
        delay(1);
    }
    FastLED.show();
}

void disco() {
    uint8_t random_delay = random(5, 100);
    uint8_t random_bool = random(0, 20);
    for (int i = 0; i < ls.quantity; i++) {
        ledsPtr[i] = (random_bool < 5) ? CRGB::White : CRGB::Black;
    }
    LEDS.show();
    delay(random_delay);
}

void rainbow() {
    for (int i = 0; i < ls.quantity; i++) {
        ledsPtr[i] = CHSV(counter + i * 2, 255, 255);  // CHSV(hue, saturation, value)
    }
    counter++;  // 0 to 255 (byte data type)
    FastLED.show();
    delay(10);  // moving speed
}

int ihue = 0;

void extractColor() {
    ls.red = 0;
    ls.green = 0;
    ls.blue = 0;
    uint8_t counter = 0;
    uint8_t value = 0;
    char charHolder;
    for (int i = 0; i < rgbLength; i++) {
        uint16_t n = 0;
        if (static_cast<char>(rgbValues[i]) != ',') {
            charHolder = rgbValues[i];
            n = charHolder - '0';
            value = value * 10 + n;
        } else {
            if (counter) {
                ls.green = value;
            } else {
                ls.red = value;
            }
            counter++;
            value = 0;
        }
    }
    ls.blue = value;
}

bool loadLsSettings() {
    readSettings("/lsconf.txt", reinterpret_cast<byte *>(&ls), sizeof(ls));

    if (ls.red || ls.green || ls.blue || ls.mode || ls.brightness) {
        return true;
    }

    return false;
}

bool saveLsSettings() { return writeSettings("/lsconf.txt", reinterpret_cast<byte *>(&ls), sizeof(ls)); }
