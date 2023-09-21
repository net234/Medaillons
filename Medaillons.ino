/*
  Medaillons.ino

   Gestion d'un bandeau WS2812 pour faire un medaillon annimé

   Copyright 20201 Pierre HENRY net23@frdev.com

   Medaillons is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bandeau is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with betaEvents.  If not, see <https://www.gnu.org/licenses/lglp.txt>.


  History
   V1.0 (30/10/2021)
   - Full rebuild from Bandeau  01/2021 net234


*************************************************/


#include <Arduino.h>
#include "nano.h"


#define APP_VERSION "Medaillon V1.0"

//enum typeEvent { evNill=0, ev100Hz, ev10Hz, ev1Hz, ev24H, evInChar,evInString,evUser=99 };
enum myEvent {
  evBP0 = 100,
  evLed0,
  evSaveDisplayMode,
  evDisplayOff,
  evStartAnim,
  evNextAnim,
};

// Gestionaire d'evenemnt
#define DEBUG_ON
#include <BetaEvents.h>
#include "WS2812.h"


// varibale modifiables
const uint8_t  ledsMAX = 7;  // nombre de led sur le bandeau
const uint8_t autoOffDelay = 60;   // delais d'auto extinction en secondes (0 = pas d'autoextinction)
// varibale modifiables (fin)

// Array contenant les leds du medaillon
WS2812rvb_t leds[ledsMAX + 1];

enum mode_t { modeOff, modeFeu, modeGlace, modeVent, modeTerre, modeLumiere, modeTenebre, MAXmode}  displayMode = modeFeu;
uint8_t displayStep = 0;
e_rvb baseColor = rvb_red;
uint16_t speedAnim = 200;

bool sleepOk = true;


#include <EEPROM.h>


void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  //Serial.begin(115200);
  //Serial.println("Bonjour");
  Events.begin();
  Serial.println(F(APP_VERSION));

  pinMode(PIN_WS2812, OUTPUT);
  for (uint8_t N = 0; N < ledsMAX; N++) {
    leds[N].setcolor(rvb_white, 80, 2000, 2000);
  }

  getDisplayMode();
}



// the loop function runs over and over again forever
void loop() {

  Events.get(sleepOk);     // get standart event
  Events.handle();  // handle standart event

  switch (Events.code) {

    case evInit:
      Serial.println(F("Init Ok"));
      if (displayMode != modeOff) {
        Events.delayedPush(5000, evStartAnim);
        if (autoOffDelay) Events.delayedPush(1000L * autoOffDelay + 5000, evDisplayOff);
      }

      break;


    case ev100Hz:
      // refresh led
      jobRefreshLeds(10);

      break;

    case evSaveDisplayMode:
      setDisplayMode();
      break;


    case evDisplayOff:
      displayMode = modeOff;
      break;

    case evStartAnim:
      startAnim();
      break;

    case evNextAnim:
      nextAnim();
      break;

    case evBP0: {

        switch (Events.ext) {

          case evxOff:
            Serial.println(F("BP0 Up"));
            break;


          case evxOn:
            mode_t oldDisplay = displayMode;
            Serial.println(F("BP0 Down"));
            if (displayMode) {
              displayMode = (mode_t)( (displayMode + 1) % MAXmode );
            } else {
              getDisplayMode();
              if (displayMode == modeOff) displayMode = modeFeu;
            }

            if (oldDisplay != displayMode) Events.delayedPush(5000, evSaveDisplayMode);
            Events.removeDelayEvent(evNextAnim);
            if (displayMode) {
              if (autoOffDelay) Events.delayedPush(1000L * autoOffDelay, evDisplayOff);
              Events.push(evStartAnim);

            }
            break;




        }
      }
      case evInChar: {
        if (Keyboard.inputChar == 'S') {
          sleepOk = !sleepOk;
          D_println(sleepOk);
        }
      }
  }


  //delay(1);
}

void startAnim() {
  displayStep = 0;
  Events.push(evNextAnim);
  switch (displayMode) {
    case modeFeu:
      baseColor = rvb_red;
      speedAnim = 300;
      break;
    case modeGlace:
      baseColor = rvb_blue;
      speedAnim = 800;
      break;
    case modeVent:
      baseColor = rvb_green;
      speedAnim = 80;
      break;
    case modeTerre:
      baseColor = rvb_orange;
      speedAnim = 300;
      break;
    case modeLumiere:
      baseColor = rvb_white;
      speedAnim = 50;
      break;
    case modeTenebre:
      baseColor = rvb_purple;
      speedAnim = 300;
      break;


  }
}

void nextAnim() {
  switch (displayMode) {
    case modeFeu:
      if (displayStep < 4) {
        leds[displayStep].setcolor(baseColor, 80, speedAnim * 1, speedAnim * 2);
        leds[7 - displayStep].setcolor(baseColor, 80, speedAnim * 1, speedAnim * 2);
      }
      break;
    case modeGlace:
      if (displayStep == 0) {
        for (uint8_t N = 0; N < ledsMAX; N++) {
          leds[N].setcolor(baseColor, 90, speedAnim * 6, speedAnim * 1);
        }

      }

      break;
    case modeVent:
      leds[displayStep].setcolor(baseColor, 100, speedAnim * 2, speedAnim * 1);
      break;

    case modeTerre:

      if (displayStep == 0) {
        for (uint8_t N = 0; N < ledsMAX; N += 2) {
          leds[N].setcolor(baseColor, 90, speedAnim * 2, speedAnim * 2);
        }
      }
      if (displayStep == 4) {
        for (uint8_t N = 1; N < ledsMAX; N += 2) {
          leds[N].setcolor(baseColor, 90, speedAnim * 2, speedAnim * 1);
        }

      }

      break;

    case modeLumiere:
      if (displayStep == 0) {
        for (uint8_t N = 0; N < ledsMAX; N++) {
          leds[N].setcolor(baseColor, 100, 50, 1);
        }

      }
      break;

    case modeTenebre:
      if (displayStep == 0) {
        for (uint8_t N = 0; N < ledsMAX; N++) {
          leds[N].setcolor(baseColor, 100, 1, speedAnim * 5);
        }

      }      break;


  }
  if (displayMode) {
    displayStep++;
    if (displayStep >= ledsMAX) displayStep = 0;
    Events.delayedPush(speedAnim, evNextAnim);
  }

}



void  getDisplayMode() {
  // lecture de  l'EEPROM pour le choix de l'animation
  displayMode = modeOff;
  // check if a stored value
  if (EEPROM.read(1) == 'B') {
    displayMode = (mode_t)EEPROM.read(2);
    TD_println("Saved Anime", displayMode);

    if (displayMode >= MAXmode ) displayMode = modeOff;
  }
}

void setDisplayMode() {
  EEPROM.update(1, 'B');
  EEPROM.update(2, displayMode);
  TD_println("Save Anime ", displayMode);
}

// 100 HZ
void jobRefreshLeds(const uint8_t delta) {
  for (int8_t N = 0; N < ledsMAX; N++) {
    leds[N].write();
  }
  leds[0].reset(); // obligatoire

  for (uint8_t N = 0; N < ledsMAX; N++) {
    leds[N].anime(delta);
  }

}
