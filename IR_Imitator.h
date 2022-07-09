#ifndef IRIMIT_H
#define IRIMIT_H

#include "FreeRTOSConfig_usr.h"
#include <LiquidCrystal.h>
#include <Arduino_FreeRTOS.h>
#include <IRremote.hpp>
#include <EEPROM.h>
#include<avr/pgmspace.h>
#include<avr/sleep.h>

//Pin setting
#define PIN_SEND 12
#define PIN_RECV 11
#define LCD_RS 9
#define LCD_ENABLE 10
#define LCD_D4 8
#define LCD_D5 7
#define LCD_D6 6
#define LCD_D7 5
#define LCD_BUCKLIGHT 4

//Protocol names, these going to stored to EEROM
//DONOT put more than 8 chars
const char Protocol_0[] PROGMEM = "UNKNOWN";
const char Protocol_1[] PROGMEM = "DISTANCE";
const char Protocol_2[] PROGMEM = "WIDTH";
const char Protocol_3[] PROGMEM = "DENON";
const char Protocol_4[] PROGMEM = "DISH";
const char Protocol_5[] PROGMEM = "JVC";
const char Protocol_6[] PROGMEM = "LG";
const char Protocol_7[] PROGMEM = "LG2";
const char Protocol_8[] PROGMEM = "NEC";
const char Protocol_9[] PROGMEM = "PANASONIC";
const char Protocol_10[] PROGMEM = "KASEIKYO";
const char Protocol_11[] PROGMEM = "JVC";
const char Protocol_12[] PROGMEM = "DENON";
const char Protocol_13[] PROGMEM = "SHARP";
const char Protocol_14[] PROGMEM = "MITUBISI";
const char Protocol_15[] PROGMEM = "RC5";
const char Protocol_16[] PROGMEM = "RC6";
const char Protocol_17[] PROGMEM = "SAMSUNG";
const char Protocol_18[] PROGMEM = "SHARP";
const char Protocol_19[] PROGMEM = "SONY";
const char Protocol_20[] PROGMEM = "ONKYO";
const char Protocol_21[] PROGMEM = "APPLE";
const char Protocol_22[] PROGMEM = "BOSEWAVE";
const char Protocol_23[] PROGMEM = "LEGO_PF";
const char Protocol_24[] PROGMEM = "MGIQUEST";
const char Protocol_25[] PROGMEM = "WHYNTER";

const char *const ProtocolList[] PROGMEM = {
  Protocol_0,
  Protocol_1,
  Protocol_2,
  Protocol_3,
  Protocol_4,
  Protocol_5,
  Protocol_6,
  Protocol_7,
  Protocol_8,
  Protocol_9,
  Protocol_10,
  Protocol_11,
  Protocol_12,
  Protocol_13,
  Protocol_14,
  Protocol_15,
  Protocol_16,
  Protocol_17,
  Protocol_18,
  Protocol_19,
  Protocol_20,
  Protocol_21,
  Protocol_22,
  Protocol_23,
  Protocol_24,
  Protocol_25
};


void TaskMain(void *pvParameters);
void TaskButtonControl(void *pvParameters);

// void LcdDisplay(bool clearDisp, bool cursorSet, char text[16]);
void LcdDisplay(bool clearDisp, bool cursorSet, char*);
void IRTransmiter(int buttonNum);
bool IRReceiver(int buttonNum);
void EEPROMControler(bool storeOrRead);
void ReadOutTextFromPRGMEM(char *returnText[5], int protocolNum);
void WakeUp();
void SetUpSleep();
bool TimeKeeper(bool timeReset);
void WDT_Prescaler_Change();

#endif
