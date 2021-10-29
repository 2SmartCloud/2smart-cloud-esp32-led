#pragma once
#include <Arduino.h>

#define BUTTON 13
const uint8_t LED_STATUS = 2;
const uint8_t ERASE_FLASH = 15;
const uint8_t RELOAD_TIME = 3;  // sec
extern bool eraseFlag;

void setGpios();
void checkButtons();
void IRAM_ATTR onTimer();
void IRAM_ATTR eraseInterrupt();
