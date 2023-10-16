void jobStartAnim() {
  displayStep = 0;
  D_println(currentAnim);
  currentMode = displayMode1;
  if (currentAnim == 2) {
    currentMode = displayMode2;
  }
  TD_print("Start Anim", currentMode);
  Events.push(evNextStep);
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

void jobNextStep() {
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
    if (displayStep < ledsMAX - 1) {
      displayStep++;
      Events.delayedPush(speedAnim, evNextStep);
    } else {
      Events.delayedPush(speedAnim, evNextAnim);
    }
  }

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
