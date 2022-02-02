#include "lenta.h"

#include "file_system/src/file_system.h"

Lenta::Lenta(const char* name, const char* id, Device* device) : Node(name, id, device) {
    if (!ReadSettings("/lentaconf.txt", reinterpret_cast<byte*>(&ls), sizeof(ls))) {
        ls.brightness_ = kDefaultBrigthness_;
        ls.state_ = true;
        ls.mode_ = RAINBOW;
        ls.red_ = kDefaultColor_;
        ls.green_ = kDefaultColor_ * 2;
        ls.blue_ = kDefaultColor_ * 3;
        ls.quantity_ = kDefaultLedsQuantity_;
    }

    leds_ptr_ = new CRGB[ls.quantity_];
    FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds_ptr_, ls.quantity_).setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, kMax_ma_);
    TurnOffLs();
    new_ls_state_ = NO_CHANGES;
    FastLED.setBrightness(ls.brightness_ * 2);
    if (ls.mode_ == COLOR) new_ls_state_ = NEW_MODE;
}

bool Lenta::Init(Homie* homie) {  // initialize toggles for notification
    bool status = true;
    if (!Node::Init(homie)) status = false;

    LoadLentaSettings();
    HandleCurrentState();

    return status;
}

void Lenta::HandleCurrentState() {
    button_.tick();
    if (button_.isPress()) {
        if (++ls.mode_ > modes_.size() - 1) ls.mode_ = 0;
        String state_in_string = modes_.find(ls.mode_)->second;
        properties_.find("mode")->second->SetValue(state_in_string);
        new_ls_state_ = NEW_MODE;
    }
    if (button_.isHolded()) {
        ls.state_ = !ls.state_;
        String state_in_string = ls.state_ ? "true" : "false";
        properties_.find("switch")->second->SetValue(state_in_string);
        new_ls_state_ = NEW_MODE;
    }

    if (properties_.find("switch")->second->HasNewValue()) {
        ls.state_ = properties_.find("switch")->second->GetValue() == "true";
        new_ls_state_ = NEW_MODE;
        properties_.find("switch")->second->SetHasNewValue(false);
    }
    if (properties_.find("mode")->second->HasNewValue()) {
        String mode_buffer = properties_.find("mode")->second->GetValue();
        for (auto it = modes_.begin(); it != modes_.end(); ++it) {
            if (it->second == mode_buffer) {
                ls.mode_ = it->first;
            }
        }
        new_ls_state_ = NEW_MODE;
        properties_.find("mode")->second->SetHasNewValue(false);
    }
    if (properties_.find("color")->second->HasNewValue()) {
        ExtractColor(properties_.find("color")->second->GetValue());

        new_ls_state_ = NEW_COLOR;
        properties_.find("color")->second->SetHasNewValue(false);
    }

    if (properties_.find("brightness")->second->HasNewValue()) {
        ls.brightness_ = properties_.find("brightness")->second->GetValue().toInt();
        new_ls_state_ = NEW_BRIGHTNESS;
        properties_.find("brightness")->second->SetHasNewValue(false);
    }

    if (properties_.find("quantity")->second->HasNewValue()) {
        ls.quantity_ = properties_.find("quantity")->second->GetValue().toInt();
        new_ls_state_ = NEW_BRIGHTNESS;
        if (SaveLentaSettings()) ESP.restart();
    }

    switch (new_ls_state_) {
        case NO_CHANGES:
            break;
        case NEW_COLOR:
            break;
        case NEW_BRIGHTNESS:
            FastLED.setBrightness(ls.brightness_ * 2);
            FastLED.show();
            break;
        case NEW_MODE:
            break;
    }

    if (ls.state_) {
        switch (ls.mode_) {
            case COLOR:
                if (new_ls_state_ == NO_CHANGES) break;
                ChangeColor();
                break;
            case RAINBOW:
                Rainbow();
                break;
            case DISCO:
                Disco();
                break;
            case GARLAND:
                Garland();
                break;
        }
    } else if (new_ls_state_) {
        TurnOffLs();
    }
    if (new_ls_state_ != NO_CHANGES) {
        last_update_time_ = millis();
        new_ls_state_ = NO_CHANGES;
        new_data_for_save_ = true;
    }
    if (millis() - last_update_time_ > kSaveLentaSettingsTime_ && new_data_for_save_) {
        SaveLentaSettings();
        new_data_for_save_ = false;
    }
}

void Lenta::PublishMode(uint8_t mode_num) {
    if (mode_num > modes_.size() - 1) return;
    ls.mode_ = mode_num;
    String state_in_string = modes_.find(ls.mode_)->second;
    properties_.find("mode")->second->SetValue(state_in_string);
}

void Lenta::TurnOffLs() {
    FastLED.clear();
    FastLED.show();
}

void Lenta::Rainbow() {
    if (leds_ptr_ == nullptr) return;
    for (int i = 0; i < ls.quantity_; i++) {
        leds_ptr_[i] = CHSV(counter_ + i * 2, 255, 255);  // CHSV(hue, saturation, value)
    }
    counter_++;  // 0 to 255 (byte data type)
    FastLED.show();
    delay(10);  // moving speed
}

void Lenta::Garland() {
    if (leds_ptr_ == nullptr) return;
    const uint8_t delay_ = 30;
    if (millis() - effects_timer_ < delay_) return;

    if (counter_ > 0 && counter_ < 85) {
        for (int i = 0; i < ls.quantity_; i += 3) {
            leds_ptr_[i] = CRGB((counter_ < 43 ? counter_ : (84 - counter_)) * 5, 0, 0);
        }
    }

    if (counter_ > 84 && counter_ < 169) {
        for (int i = 1; i < ls.quantity_; i += 3) {
            leds_ptr_[i] = CRGB(0, (counter_ < 127 ? (counter_ - 84) : (168 - counter_)) * 5, 0);
        }
    }

    if (counter_ > 168 && counter_ < 253) {
        for (int i = 2; i < ls.quantity_; i += 3) {
            leds_ptr_[i] = CRGB(0, 0, (counter_ < 211 ? (counter_ - 168) : (252 - counter_)) * 5);
        }
    }
    counter_++;
    if (counter_ >= 253) counter_ = 0;
    FastLED.show();
    effects_timer_ = millis();
}

void Lenta::ExtractColor(String color_string) {
    ls.red_ = color_string.substring(0, color_string.indexOf(',')).toInt();
    ls.green_ = color_string.substring(color_string.indexOf(',') + 1, color_string.lastIndexOf(',')).toInt();
    ls.blue_ = color_string.substring(color_string.lastIndexOf(',') + 1).toInt();
}

void Lenta::ChangeColor() {
    Serial.println("in change color");
    for (int led = 0; led < ls.quantity_; led++) {
        leds_ptr_[led] = CRGB(ls.red_, ls.green_, ls.blue_);
        delay(1);
    }
    FastLED.show();
}

void Lenta::Disco() {
    uint8_t random_delay = random(5, 100);
    uint8_t random_bool = random(0, 20);
    for (int i = 0; i < ls.quantity_; i++) {
        leds_ptr_[i] = (random_bool < 5) ? CRGB::White : CRGB::Black;
    }
    LEDS.show();
    delay(random_delay);
}

String Lenta::GetModes() {
    String ModeFormatValue = "";
    for (auto it = modes_.begin(); it != modes_.end(); ++it) {
        ModeFormatValue += it->second;
        ModeFormatValue += ",";
    }
    return ModeFormatValue.substring(0, ModeFormatValue.length() - 1);
}

bool Lenta::LoadLentaSettings() {
    ReadSettings("/lentaconf.txt", reinterpret_cast<byte*>(&ls), sizeof(ls));

    String state_in_string = ls.state_ ? "true" : "false";
    properties_.find("switch")->second->SetValue(state_in_string);
    properties_.find("switch")->second->SetHasNewValue(false);

    state_in_string = modes_.find(ls.mode_)->second;
    properties_.find("mode")->second->SetValue(state_in_string);
    properties_.find("mode")->second->SetHasNewValue(false);

    properties_.find("brightness")->second->SetValue(String(ls.brightness_));
    properties_.find("brightness")->second->SetHasNewValue(false);

    char message_buffer[12];  // length of RGB mess

    snprintf(message_buffer, sizeof(message_buffer), "%d,%d,%d", ls.red_, ls.green_, ls.blue_);
    properties_.find("color")->second->SetValue(message_buffer);
    properties_.find("color")->second->SetHasNewValue(false);

    properties_.find("quantity")->second->SetValue(String(ls.quantity_));
    properties_.find("quantity")->second->SetHasNewValue(false);

    return true;
}

bool Lenta::SaveLentaSettings() { return WriteSettings("/lentaconf.txt", reinterpret_cast<byte*>(&ls), sizeof(ls)); }
