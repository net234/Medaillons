/**************************************************************************************
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
   V1.1 (29/09/2022)
   - Ajustement pour version Meluze

*************************************************/


#include <Arduino.h>
#include "nano.h"


#define APP_VERSION "Medaillon V1.1"

//enum typeEvent { evNill=0, ev100Hz, ev10Hz, ev1Hz, ev24H, evInChar,evInString,evUser=99 };
enum myEvent {
  evBP0 = 100,
  evLed0,
  //evSaveDisplayMode,
  evDisplayOff,
  evStartAnim,
  evNextAnim,
  evAckRadio,
  evWhoIsHere,
  evIamHere,
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





enum mode_t { modeOff, modeFeu, modeGlace, modeVent, modeTerre, modeLumiere, modeTenebre, maxMode}  currentMode = modeFeu;
uint8_t displayStep = 0;  // Etape en cours dans l'anime
e_rvb baseColor = rvb_red;  // Couleur base de l'animation
//uint16_t dureeAnim
uint16_t speedAnim = 200;
mode_t displayMode1;
mode_t displayMode2;
byte currentAnime;  // 0 a l'init 1 pour l'anime1  2 pour l'anime2

bool sleepOk = true;


WS2812rvb_t leds[ledsMAX + 1];

#include <EEPROM.h>


void setup() {

  //Serial.begin(115200);
  //Serial.println("Bonjour");
  Events.begin();
  Serial.println(F(APP_VERSION));

  pinMode(PIN_WS2812, OUTPUT);
  for (uint8_t N = 0; N < ledsMAX; N++) {
    leds[N].setcolor(rvb_white, 80, 2000, 2000);
  }

  getDisplayMode();

  nrfSetup();
}



// the loop function runs over and over again forever
void loop() {
  nrfHandle();
  Events.get(sleepOk);     // get standart event
  Events.handle();  // handle standart event

  switch (Events.code) {

    case evInit:
      Serial.println(F("Init Ok"));
      Events.delayedPush(5000, evStartAnim);
      if (autoOffDelay) Events.delayedPush(1000L * autoOffDelay + 5000, evDisplayOff);

      break;


    case ev100Hz:
      // refresh led
      jobRefreshLeds(10);

      break;




    case evDisplayOff:
      currentMode = modeOff;
      break;

    case evStartAnim:
      if (autoOffDelay) Events.delayedPush(1000L * autoOffDelay, evDisplayOff);
      startAnim();
      break;

    case evNextAnim:
      nextAnim();
      break;


    case evIamHere:
      Serial.println(F("Send evIamHere"));
      nrfSend(evIamHere);
      break;


    case evBP0: {

        switch (Events.ext) {

          case evxOff:
            Serial.println(F("BP0 Up"));
            break;


          case evxOn:

            Serial.println(F("BP0 Down"));
            if (currentMode) {
              currentMode = modeOff;
            } else {
              currentMode = displayMode1;
            }

            Events.removeDelayEvent(evNextAnim);
            if (currentMode) Events.push(evStartAnim);
            TD_println("Current Mode ", currentMode);
            nrfSend(1);


            break;

          case evxLongOn:
            Serial.println(F("BP0 Long On"));
            saveDisplayMode();
            break;


        }
        break;
      }
    case evAckRadio: {
        nrfAck();
        break;
      }
    case evWhoIsHere: {
        nrfSend(evWhoIsHere);
        break;
      }

    case evInChar: {
        if (Keyboard.inputChar == '1') {
          displayMode1 = modeLumiere;
          displayMode2 = modeVent;
        }

        if (Keyboard.inputChar == '2') {
          displayMode1 = modeTenebre;
          displayMode2 = modeTerre;
        }

      
        if (Keyboard.inputChar == 'S') {
          sleepOk = !sleepOk;
          D_println(sleepOk);
        }
        if (Keyboard.inputChar == 'W') {

          T_println("Who ?");
          Events.push(evWhoIsHere);
        }

        if (Keyboard.inputChar == 'P') {

          TD_println("Current Mode ", currentMode);
          nrfSend(1);
        }

        break;
      }
  }


  //delay(1);
}

void startAnim() {
  displayStep = 0;
  D_println(currentAnime);
  currentAnime++;
  currentMode = displayMode1;
  if (currentAnime == 2) {
    currentAnime = 0;
    if (displayMode2) currentMode = displayMode2;
  }
  TD_print("Start Anim",currentMode);
  D_println(currentAnime);
  Events.push(evNextAnim);
  switch (currentMode) {
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
      baseColor = rvb_brown;
      speedAnim = 300;
      break;
    case modeLumiere:
      baseColor = rvb_white;
      speedAnim = 300;
      break;
    case modeTenebre:
      baseColor = rvb_purple;
      speedAnim = 500;
      break;


  }
}

void nextAnim() {
  //D_println(displayStep);
  switch (currentMode) {
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
      leds[6 - displayStep].setcolor(baseColor, 100, speedAnim * 2, speedAnim * 1);

      //      if (displayStep == 0) {
      //        for (uint8_t N = 0; N < ledsMAX; N++) {
      //          leds[N].setcolor(baseColor, 100,  speedAnim * 5, 1);
      //        }
      //
      //     }
      break;

    case modeTenebre:
      if (displayStep == 0) {
        for (uint8_t N = 0; N < ledsMAX; N++) {
          leds[N].setcolor(baseColor, 100, speedAnim, speedAnim * 5);
        }

      }      break;


  }
  if (currentMode) {
    if (displayStep < ledsMAX-1) {
      displayStep++;
      Events.delayedPush(speedAnim, evNextAnim);
    } else {
      Events.delayedPush(speedAnim, evStartAnim);
    }
  }

}



void  getDisplayMode() {
  // lecture de  l'EEPROM pour le choix de l'animation
  currentMode = 0;
  displayMode1 = modeLumiere;
  displayMode2 = modeOff;
  // check if a stored value
  if (EEPROM.read(1) == 'B') {
    displayMode1 = (mode_t)EEPROM.read(2);
    if (displayMode1 == 0 or displayMode1 > maxMode) displayMode1 = modeLumiere;
    TD_println("Saved displayMode1", displayMode1);
    displayMode2 = (mode_t)EEPROM.read(3);
    if (displayMode2 > maxMode) displayMode2 = modeOff;
    TD_println("Saved displayMode2", displayMode2);
  }
}

void saveDisplayMode() {
  EEPROM.update(1, 'B');
  EEPROM.update(2, displayMode1);
  TD_println("Save displayMode1 ", displayMode1);
  EEPROM.update(3, displayMode2);
  TD_println("Save displayMode2 ", displayMode2);
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
