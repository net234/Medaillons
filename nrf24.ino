#include "RF24.h"   // version modifiee pour acceder a D13

// nom du canal pour la com
const byte radioname[6] = "Meda1";
//construction  d'un uuid unique a partir de l'heure et la date de compil
#define BUILDTIME __TIME__ " " __DATE__
const byte buidtime[] = BUILDTIME;  //BUILDTIME => '15:52:28 Sep 24 2023
const unsigned int  appUID = ((buidtime[6] + buidtime[7]) & 0x0F) |        // sec
                             ( ((buidtime[3] + buidtime[4]) & 0x0F) << 4) |  // min
                             ( ((buidtime[0] + buidtime[1]) & 0x0F) << 8) |  // heu
                             ( ((buidtime[13] + buidtime[14]) & 0x0F) << 12) ; // day


//* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 10 & 9 * /
RF24 radio(10, 9);

byte messNum = 0;


struct RadioEvent  {
  byte  num;
  unsigned int  who;
  unsigned int  from;

  byte  what;
  byte mode1;
  byte mode2;
};


RadioEvent receveRadioEvent;
RadioEvent transmitRadioEvent;

void nrfSetup() {
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  //radio.setPALevel(RF24_PA_LOW);
  radio.setPALevel(RF24_PA_HIGH);

  radio.disableDynamicPayloads();
  radio.setChannel(109);
  radio.setPayloadSize(sizeof(RadioEvent));
  Serial.print("Payload Size:");
  Serial.print(sizeof(RadioEvent));
  D_print(appUID);
  D_print(BUILDTIME);
  Serial.println();
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(false);
  // radio.maskIRQ(0, 0, 0);
  // Start the radio listening for data
  //W_TX_PAYLOAD_NO_ACK write command?


  radio.openWritingPipe(radioname);
  radio.openReadingPipe(1, radioname);
  radio.startListening();

  if (radio.isChipConnected()) {
    Serial.print("NRF24L01");
    if (radio.isPVariant()) {
      Serial.print("+");
    }
    Serial.println(" connecte.");
  } else {
    Serial.println("!! NRF24L01 ABSENT");
  }
  radio.disableSPI();
  messNum = 0;
}



// Verifie la presence d'eventRadio recu
// En cas de reception l'event est mis dans la boucle Event standard et un ack est renvoyé avec un delay de nodeRank
void nrfHandle() {

  // reprise en main du SPI
  bool SD13 = digitalRead(13);
  radio.enableSPI();

  // Si un evenemnt est recu
  if (radio.available())  {
    //Serial.println("Radio");
    //  digitalWrite(6, true);
    radio.read( &receveRadioEvent, sizeof(receveRadioEvent) );

    Serial.print("Radio\r\n[<=");
    D_print(receveRadioEvent.num);
    TD_print("Who", receveRadioEvent.who);
    TD_print("From", receveRadioEvent.from);
    TD_print("What", receveRadioEvent.what);
    TD_print("Mode1", receveRadioEvent.mode1);
    TD_print("Mode2", receveRadioEvent.mode2);


    Serial.print("] ");
    if (receveRadioEvent.who == appUID) {
      Serial.println("ack");
    } else {
      if (receveRadioEvent.num == messNum) {
        Serial.println("dub");
      } else {
        messNum = receveRadioEvent.num;
        if (receveRadioEvent.what == 1 or receveRadioEvent.what == 0) {
          displayMode1 = receveRadioEvent.mode1;
          displayMode2 = receveRadioEvent.mode2;
          currentMode = 0;
          if (receveRadioEvent.what) currentMode = receveRadioEvent.mode1;
          TD_print("Distant Mode1", displayMode1);
          TD_println("Mode2", displayMode2);
          Events.removeDelayEvent(evNextAnim);
          if (currentMode) Events.push(evStartAnim);


        }
        if (receveRadioEvent.what == evWhoIsHere) {

          TD_println("Request Who is there", appUID);
          Events.delayedPush(random(110, 150), evIamHere);
        }
        Events.delayedPush(random(2, 11), evAckRadio);
      }
    }

  }
  radio.disableSPI();
  digitalWrite(13, SD13);
  // digitalWrite(6, false);
}

void nrfAck() {
  Serial.println("send ack");
  bool SD13 = digitalRead(13);
  radio.enableSPI();
  receveRadioEvent.from = appUID;
  radio.stopListening();
  radio.write( &receveRadioEvent, sizeof(receveRadioEvent), true );
  radio.startListening();
  radio.disableSPI();
  digitalWrite(13, SD13);

}

void  nrfSend(byte what) {
  TD_print("Send UUID", appUID);
  TD_print(, what);
  transmitRadioEvent.num = ++messNum;
  transmitRadioEvent.who = appUID;
  transmitRadioEvent.from = appUID;
  transmitRadioEvent.what = what;
  transmitRadioEvent.mode1 = displayMode1;
  transmitRadioEvent.mode2 = displayMode2;
  bool SD13 = digitalRead(13);
  radio.enableSPI();

  radio.stopListening();
  radio.write( &transmitRadioEvent, sizeof(transmitRadioEvent), true );
  radio.startListening();
  radio.disableSPI();
  digitalWrite(13, SD13);
}
