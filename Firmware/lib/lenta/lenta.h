#pragma once

#include <Arduino.h>
#include <EncButton.h>
#include <FastLED.h>

#include <map>
#include <string>

#include "homie.h"

#define DATA_PIN 18

class Lenta : public Node {
 public:
    Lenta(const char* name, const char* id, Device* device);

    bool Init(Homie* homie);

    void HandleCurrentState();

    String GetModes();

    void PublishMode(uint8_t mode_num);

 private:
    typedef struct {
        bool state_;    // on/off
        uint8_t mode_;  // rainbow/disco/color ...
        uint8_t brightness_;
        uint8_t red_;
        uint8_t green_;
        uint8_t blue_;
        uint16_t quantity_;
    } LsSettings;

    enum LsNewState { NO_CHANGES, NEW_COLOR, NEW_BRIGHTNESS, NEW_MODE };
    enum LedStripStates { RAINBOW, COLOR, DISCO, GARLAND };

    void TurnOffLs();
    void Rainbow();
    void Disco();
    void Garland();
    void ChangeColor();
    void ExtractColor(String color_string);

    bool SaveLentaSettings();
    bool LoadLentaSettings();

    const uint16_t kMax_ma_ = 1500;  // max mA, for library

    const uint8_t kDefaultBrigthness_ = 10;
    const uint8_t kDefaultColorR_ = 50;
    const uint8_t kDefaultColorG_ = 200;
    const uint8_t kDefaultColorB_ = 200;
    const uint16_t kDefaultLedsQuantity_ = 300;         // 5m led strip
    const uint16_t kSaveLentaSettingsTime_ = 5 * 1000;  // 5s

    LsSettings ls = {1, 0, 20, kDefaultColorR_, kDefaultColorG_, kDefaultColorB_, 300};

    bool new_data_for_save_ = false;

    CRGB* leds_ptr_ = nullptr;

    uint8_t new_ls_state_ = 0;
    uint8_t counter_ = 0;
    uint32_t period_loop_ = millis();
    uint32_t last_update_time_ = 0;
    uint32_t effects_timer_ = 0;

    std::map<uint8_t, String> modes_ = {{RAINBOW, "rainbow"}, {COLOR, "color"}, {DISCO, "disco"}, {GARLAND, "garland"}};

    EncButton<EB_TICK, 13> button_;
};
