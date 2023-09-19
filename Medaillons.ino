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
#include "WS2812.h"

uint8_t div10Hz = 10;
uint8_t div1Hz = 10;
uint16_t divAnime = 250;

// varibale modifiables
const uint8_t  ledsMAX = 7;  // nombre de led sur le bandeau
const uint16_t autoOffDelay = 60;   // delais d'auto extinction en secondes (0 = pas d'autoextinction)
// varibale modifiables (fin)

// Array contenant les leds du medaillon
WS2812rvb_t leds[ledsMAX + 1];

uint16_t delayModeOff = autoOffDelay;
enum mode_t { modeOff, modeFeu, modeGlace, modeVent, modeTerre, modeLumiere, modeTenebre, MAXmode}  displayMode = modeFeu;
uint8_t displayStep = 0;
uint8_t delayMemo = 0;

#include <EEPROM.h>


void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  Serial.begin(115200);
  Serial.println("Bonjour");
  pinMode(LED_LIFE, OUTPUT);
  pinMode(PIN_BP0, INPUT_PULLUP);
  pinMode(PIN_WS2812, OUTPUT);
  for (uint8_t N = 0; N < ledsMAX; N++) {
    leds[N].setcolor(rvb_white, 80, 1000, 1000);
  }

  // lecture de  l'EEPROM pour le choix de l'animation
  // check if a stored value
  if (EEPROM.read(1) == 'B') {
    displayMode = (mode_t)EEPROM.read(2);
    Serial.print("Saved Anime ");
    Serial.println(displayMode);

    if (displayMode >= MAXmode ) displayMode = modeOff;
    EEPROM.write(1, 'B');
    EEPROM.write(2, displayMode);
  }
}

uint32_t milli1 = millis();  // heure du dernier 100Hz obtenus
bool ledLifeStat = true;
bool bp0Stat = false;

// the loop function runs over and over again forever
void loop() {

  // detection de 10milliseconde pour btenir du 100Hz
  uint16_t delta = millis() - milli1;
  if (  delta >= 10 ) {
    milli1 += 10;

    // 100 Hzt rafraichissement bandeau
    jobRefreshLeds(10);

    // diviseur pour avoir du 10Hz
    if (--div10Hz == 0) {
      // 10 Hzt
      div10Hz = 10;

      //10HZ test poussoir
      jobPoussoir();

      // diviseur pour avoir du 1HZ
      if (--div1Hz == 0) {
        div1Hz = 10;

        // job 1 Hz
        ledLifeStat = !ledLifeStat;
        digitalWrite(LED_LIFE, ledLifeStat);   // turn the LED on (HIGH is the voltage level)

        if (delayModeOff) {
          if (--delayModeOff == 0) {
            displayMode = modeOff;
          }
        }
        if (delayMemo) {
          delayMemo--;
          if (delayMemo == 0) {
            EEPROM.update(1, 'B');
            EEPROM.update(2, displayMode);
            Serial.print("Save Anime ");
            Serial.println(displayMode);
          }
        }

      }  // 1Hz
    } // 10Hz


    // annimation toute
    if (--divAnime == 0) {
      divAnime = 200;  // change de led chaque divanime centime de sec

      // animation
      if (displayStep < ledsMAX) {
        switch (displayMode) {
          case modeOff:
            leds[displayStep].setcolor(rvb_black, 50);
            break;
          case modeFeu:

            leds[displayStep].setcolor(rvb_red, 80, 2000, 2000);

            break;
          case modeGlace:
            leds[displayStep].setcolor(rvb_blue, 80, 4000, 4000);
            break;
          case modeVent:
            leds[displayStep].setcolor(rvb_green, 80, 100, 100);
            break;
        }
      }
      displayStep = (displayStep + 1) % (ledsMAX);

    }
  } // 100Hz

  //delay(1);
}



void jobPoussoir() {
  if ( (digitalRead(PIN_BP0) == LOW) != bp0Stat ) {
    bp0Stat = !bp0Stat;
    if (bp0Stat) {
      delayMemo = 5;
      Serial.print("delay memo");
    } else {
      displayMode = (mode_t)( (displayMode + 1) % 4 );
      delayModeOff = autoOffDelay;
      displayStep = 0;
      delayMemo = 0;
    }


  }
}


// 100 HZ
void jobRefreshLeds(const uint8_t delta) {
  for (int8_t N = 0; N < ledsMAX; N++) {
    leds[N].write();
  }
  //  option mode mirroir
  //  for (int8_t N = ledsMAX - 1; N > 0; N--) {
  //    leds[N].write();
  //  }
  leds[0].reset(); // obligatoire

  for (uint8_t N = 0; N < ledsMAX; N++) {
    leds[N].anime(delta);
  }

}
