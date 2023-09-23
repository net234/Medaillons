#include "RF24.h"   // version modifiee pour acceder a D13

// nom du canal pour la com
const byte radioname[6] = "Meda1";
//construction  d'un uuid unique a partir de l'heure et la date de compil
#define BUILDTIME __TIME__ " " __DATE__
const byte buidtime[] = BUILDTIME;
const unsigned int  appUID = (buidtime[7] & 0x0F) | ((buidtime[4] & 0x0F) << 4) | ((buidtime[1] & 0x0F) << 8) | ((buidtime[14] & 0x0F) << 12) ;  //ID:33289



//* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 10 & 9 * /
RF24 radio(10, 9);

struct RadioEvent  {
  unsigned int  who;
  byte  what;
  int action;
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
  Serial.println();
  // radio.setDataRate(RF24_250KBPS);
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
    
        Serial.print("\r\n[<=");
        D_print(receveRadioEvent.who);
        D_print(receveRadioEvent.what);
        Serial.print("]");
     if (receveRadioEvent.what == 1) {
      displayMode = receveRadioEvent.action;
      Events.push(evStartAnim);
     }

    }
  radio.disableSPI();
  digitalWrite(13, SD13);
  // digitalWrite(6, false);
}


void  nrfSend(byte what) {

  transmitRadioEvent.who = appUID;
  transmitRadioEvent.what = what;
  transmitRadioEvent.action = displayMode;
  bool SD13 = digitalRead(13);
  radio.enableSPI();

  radio.stopListening();
  radio.write( &transmitRadioEvent, sizeof(transmitRadioEvent), true );
  radio.startListening();
  radio.disableSPI();
  digitalWrite(13, SD13);
}
