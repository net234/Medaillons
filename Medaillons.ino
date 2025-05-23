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
   V1.2 (29/09/2022)
   - Ajout de la possibilité d'utiliser la magie des ténèbres et lumineuse en magie secondaire.
   - Ajustement de la magie des ténèbres et des durées des animations des différentes magies

*************************************************/


#include <Arduino.h>
#include "nano.h"


#define APP_VERSION "Medaillon V1.2"

//enum typeEvent { evNill=0, ev100Hz, ev10Hz, ev1Hz, ev24H, evInChar,evInString,evUser=99 };
enum myEvent {
  evBP0 = 100,
  evLed0,
  //evSaveDisplayMode,
  evDisplayOff,  //Extinction
  evStartAnim,   //Allumage Avec l'animation Mode1 et activation de l'extinction automatique
  evNextAnim,    //L'anime est fini on repete ou on fait la suivante
  evNextStep,    //etape suivante dans l'animation
  //evModeProg1,    // choix du programe1
  evSendModeRadio, // send actual mode
  evAckRadio,    //Ack d'un event radio
  evWhoIsHere,   //Demande radio d'identification
  evIamHere,     //Identification par radio
};

// Gestionaire d'evenemnt
#define DEBUG_ON
#include <BetaEvents.h>
#include "WS2812.h"


// varibale modifiables
const uint8_t ledsMAX = 7;  // nombre de led sur le bandeau
const uint8_t autoOffDelay = 60;   // delais d'auto extinction en secondes (0 = pas d'autoextinction)
// varibale modifiables (fin)







enum mode_t : byte { modeOff, modeFeu, modeGlace, modeVent, modeTerre, modeLumiere, modeTenebre, maxMode}  currentMode = modeFeu;
uint8_t displayStep = 0;  // Etape en cours dans l'anime (de 0 a ledsMax-1)
e_rvb baseColor = rvb_red;  // Couleur base de l'animation
uint16_t speedAnim = 200;

byte currentAnim;  // 0 a l'init 1 pour l'anime1  2 pour l'anime2
int  multiPush = -1;
bool sleepOk = true;
int modeProg = 0;

mode_t displayMode1;
mode_t displayMode2;


// Array contenant les leds du medaillon
WS2812rvb_t leds[ledsMAX];
// deux leds en rab pour la programmation
//WS2812rvb_t ledsM1;
//WS2812rvb_t ledsM2;
#include <EEPROM.h>


void setup() {

  //Serial.begin(115200);
  //Serial.println("Bonjour");
  Events.begin();
  Serial.println(F(APP_VERSION));


  //  toute les led a blanc a l'init
  pinMode(PIN_WS2812, OUTPUT);
  for (uint8_t N = 0; N < ledsMAX; N++) {
    leds[N].setcolor(rvb_white, 80, 2000, 2000);
  }

  // recup des modes memorises (mode1 et mode2)
  getDisplayMode();

  // init du module radio
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
      multiPush = -1;
      modeProg = 0;
      Events.delayedPush(5000, evStartAnim);


      break;


    case ev100Hz:
      // refresh led
      jobRefreshLeds(10);

      break;




    case evDisplayOff:
      currentMode = modeOff;
      Events.removeDelayEvent(evNextAnim);
      modeProg = 0;
      break;




    // mise en route des animations
    case evStartAnim:
      if (autoOffDelay) Events.delayedPush(1000L * autoOffDelay, evDisplayOff);
      currentAnim = 0;
      jobStartAnim();
      TD_println("Started Anim", currentMode);
      break;

    // passage a l'animation suivante
    case evNextAnim:
      currentAnim++;
      if (currentAnim >= 3) currentAnim = 1;
      if (currentAnim == 2) {
        if (!displayMode2) currentAnim = 1;
      }
      jobStartAnim();
      TD_println("nexted Anim", currentMode);
      break;

    case evNextStep:
      jobNextStep();
      break;

    case evIamHere:
      Serial.println(F("Send evIamHere"));
      nrfSend(evIamHere);
      break;


    case evBP0: {

        switch (Events.ext) {

          case evxOff:
            Serial.println(F("BP0 Up"));
            if (modeProg == 1) {
              D_println(displayMode1);
              displayMode1 = (mode_t)displayMode1 + 1;
              if (displayMode1 >= maxMode) displayMode1 = modeFeu;
              D_println(displayMode1);
              jobStartAnim();
              break;
            }
            if (modeProg == 2) {
              D_println(displayMode2);
              displayMode2 = (mode_t)displayMode2 + 1;
              if (displayMode2 >= maxMode) displayMode2 = modeOff;
              D_println(displayMode2);
              jobStartAnim();
            }


            break;


          case evxOn:

            Serial.print(F("BP0 Down "));
            if (modeProg) {
              Serial.println(F("skiped"));
              break;
            }


            multiPush++;
            D_println(multiPush);

            if (multiPush == 1 or multiPush == 2) {
              if (currentMode) {
                currentMode = modeOff;
              } else {
                currentMode = displayMode1;
              }
              Events.removeDelayEvent(evNextAnim);
              if (currentMode) Events.push(evStartAnim);

            }
            if (multiPush == 2) {
              Events.push(evSendModeRadio);
            }

            break;

          case evxLongOn:
            Serial.println(F("BP0 Long Down"));
            //saveDisplayMode();
            D_println(modeProg);
            if (modeProg == 0) {
              modeProg = 1;
              Events.push(evStartAnim);
              break;
            }
            if (modeProg == 1 and displayMode1 != modeLumiere and displayMode1 != modeTenebre ) {
              displayMode2 = modeOff;
              saveDisplayMode();
              modeProg = 0;
              Events.push(evStartAnim);
              break;
            }
            if (modeProg == 1) {
              modeProg = 2;
              Events.push(evStartAnim);
              break;
            }
            if (modeProg >= 2 ) {

              saveDisplayMode();
              modeProg = 0;
              Events.push(evStartAnim);
              break;
            }
            break;

          case evxLongOff:
            Serial.println(F("BP0 Long Off"));
            multiPush = 0;
            break;



        }
        break;
      }

    case evSendModeRadio: {
        TD_println("Send Current Mode ", currentMode);
        nrfSend( (currentMode) ? 1 : 0) ;
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
