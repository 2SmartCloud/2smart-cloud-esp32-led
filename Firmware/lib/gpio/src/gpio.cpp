#include "gpio.h"

#include "ledStrip.h"

#define NOISE_FILTER 300

uint64_t lastClickButton = 0;
hw_timer_t* timer = NULL;
bool btnPressed = false;

void IRAM_ATTR onTimer() {
    if (!digitalRead(ERASE_FLASH)) {
        eraseFlag = true;
    }
    timerAlarmDisable(timer);
    timerEnd(timer);
}

void IRAM_ATTR eraseInterrupt() {
    timer = timerBegin(0, 80, true);  // 80=prescaller 80Mhz/prescaller = 1 000 000 counts in a sec
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, RELOAD_TIME * 1000000, false);  // timer overflow in (reload time) seconds
    timerAlarmEnable(timer);
}

void setGpios() {
    pinMode(ERASE_FLASH, INPUT_PULLUP);
    attachInterrupt(ERASE_FLASH, eraseInterrupt, FALLING);
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, LOW);
    pinMode(BUTTON, INPUT_PULLUP);
}

void checkButtons() {
    if (!digitalRead(BUTTON)) {
        if (btnPressed && millis() - lastClickButton > LS_OFF_TIME) {
            newLsState = NEW_MODE;
            if (ls.state) newLsMqttData = true;
            ls.state = false;
        }
        if (!btnPressed && millis() - lastClickButton > NOISE_FILTER) {
            lastClickButton = millis();
            newLsState = NEW_MODE;
            if (++ls.mode > lsModes.size() - 1) ls.mode = 0;
            btnPressed = true;
            newLsMqttData = true;
            if (!ls.state) stateChanged = true;
            ls.state = true;
        }
    }
    if (digitalRead(BUTTON)) btnPressed = false;
}
