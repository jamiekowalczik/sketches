#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

/*
NEED TO UPDATE THIS.  EXAMPLE TAKEN FROM ANOTHER SITE, WIRING IS NOT ACCURATE FOR MY SETUP
7 Segment (Common Anode) Display Map: (This can be altered to reflect your HW)

    D   E  5V   F   G
 ___|___|___|___|___|____
|                        |
|        F               |
|    E       G           |
|        D               |
|    A       C           |
|        B       H(Dot)  |
|________________________|
    |   |   |   |   |
    A   B  5V   C   H

74HC595 Map:
           _______
   OUT Q1-|1 *  16|-5V
   OUT Q2-|2    15|-OUT Q0
   OUT Q3-|3    14|-ARD PIN 11
   OUT Q4-|4    13|-GND
   OUT Q5-|5    12|-ARD PIN 8 ; 1uF TO GND
   OUT Q6-|6    11|-ARD PIN 12 
   OUT Q7-|7    10|-5V
      GND-|8_____9|-

********************************************************************
*/

//Binary values for 0-9 & . on 7 segment LED hooked
int numsArray[11] = {63,6,91,79,102,109,125,7,127,103,128};

//Pins used for led1 on a 595
int latchPinLed1 = 2; //pin 12 on the 595
int dataPinLed1 = 3; //pin 14 on the 595
int clockPinLed1 = 4; //pin 11 on the 595

//Pins used for led2 on a 595
int latchPinLed2 = 5; //pin 12 on the 595
int dataPinLed2 = 6; //pin 14 on the 595
int clockPinLed2 = 8; //pin 11 on the 595

//Pin & vars for push button.  
//May have something backwards, 
//using 1 whereas many examples used 0.  
//Works so sticking with it for now.
int button1InPin = 7; 
int button1Val = 1;
int lastButton1Val = 1;
int curValueLed1 = 0;

int button2InPin = A0; 
int button2Val = 1;
int lastButton2Val = 1;
int curValueLed2 = 0;

int button3InPin = A2; 
int button3Val = 1;
int lastButton3Val = 1;
int curValueButton3 = 0;

//Vars for reading & writing settings
byte xeeLed1;
byte xeeLed2;
byte xeeRelay1;
int addrLed1 = 1;
int addrLed2 = 2;
int addrRelay1 = 3;

//Pin that relay is hooked up to. 
int relay = A1;
int relayVal = 0;

RF24 radio(9,10);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void writeNumLed1(int i) {
  digitalWrite(latchPinLed1,LOW); //ground latchPin and hold low for as long as you are transmitting
  shiftOut(dataPinLed1,clockPinLed1,MSBFIRST,numsArray[i]);
  digitalWrite(latchPinLed1,HIGH); //pull the latchPin to save the data
}

void writeNumLed2(int i) {
  digitalWrite(latchPinLed2,LOW); //ground latchPin and hold low for as long as you are transmitting
  shiftOut(dataPinLed2,clockPinLed2,MSBFIRST,numsArray[i]);
  digitalWrite(latchPinLed2,HIGH); //pull the latchPin to save the data
}

void saveConfigLed1(int i) {
  xeeLed1 = byte(i);
  EEPROM.write(addrLed1,xeeLed1);
}

void saveConfigLed2(int i) {
  xeeLed2 = byte(i);
  EEPROM.write(addrLed2,xeeLed2);
}

void saveConfigRelay1(int i) {
  xeeRelay1 = byte(i);
  EEPROM.write(addrRelay1,xeeRelay1);
}

void readConfig() {
  xeeLed1 = EEPROM.read(addrLed1);
  curValueLed1 = int(xeeLed1);
  if (curValueLed1 < 0 || curValueLed1 >10){
     curValueLed1 = 0;
  }

  xeeLed2 = EEPROM.read(addrLed2);
  curValueLed2 = int(xeeLed2);
  if (curValueLed2 < 0 || curValueLed2 >10){
     curValueLed2 = 0;
  }

  xeeRelay1 = EEPROM.read(addrRelay1);
  relayVal = int(xeeRelay1);
  if (relayVal == 0) {
     digitalWrite(relay, LOW);
  }else if(relayVal == 1) {
     digitalWrite(relay, HIGH);
  }
}

int getState(unsigned short piid, unsigned short deviceid){
  if( piid == curValueLed1 && deviceid == curValueLed2 ){
     boolean state = digitalRead(relay);
     return state == true ? 1: 0;
  }
}

void doAction(unsigned short piid, unsigned short deviceid, unsigned short action){
    if( piid == curValueLed1 && deviceid == curValueLed2 ){
       if( action == 0 ){
           digitalWrite(relay, LOW);
           saveConfigRelay1(0);
       }else if( action == 1){
           digitalWrite(relay, HIGH);
           saveConfigRelay1(1);
       }
    }
}

void sendCallback(unsigned short callback){
   // First, stop listening so we can talk
   radio.stopListening();

   // Send the final one back.
   radio.write( &callback, sizeof(unsigned short) );
   printf("Sent response.\n\r");

   // Now, resume listening so we catch the next packets.
   radio.startListening();
}

void performAction(unsigned short rawMessage){
   unsigned short action, piid, deviceid, callback;
   
   int input_number = int(rawMessage);
   action = (input_number%10);
   deviceid = ((input_number/10)%10);
   piid = ((input_number/100)%10);

   if (action == 0 || action ==1){
      callback = action;
      doAction(piid, deviceid, action);
   }else if(action == 2){
      callback = getState(piid, deviceid);
   }
   sendCallback(callback);
}

void performActionManually(){
  unsigned short action;
  action = digitalRead(relay);
  if( action == 0 ){
     digitalWrite(relay, HIGH);
     saveConfigRelay1(1);
  }else if( action == 1){
     digitalWrite(relay, LOW);
     saveConfigRelay1(0);
  }
}

void setup() {
  pinMode(latchPinLed1, OUTPUT);
  pinMode(dataPinLed1, OUTPUT);
  pinMode(clockPinLed1, OUTPUT);

  pinMode(latchPinLed2, OUTPUT);
  pinMode(dataPinLed2, OUTPUT);
  pinMode(clockPinLed2, OUTPUT);
  
  pinMode(button1InPin, INPUT);
  digitalWrite(button1InPin, HIGH);
  pinMode(button2InPin, INPUT);
  digitalWrite(button2InPin, HIGH);
  
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  
  readConfig();

  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("\nRemote Switch Arduino\n\r");

  //
  // Setup and configure rf radio
  //

  radio.begin();
  //  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.setRetries(15,15);

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  radio.printDetails();
}

void lookForConfigChangesLed1() {
  int arrNumsLength = sizeof(numsArray)/sizeof(int);
  button1Val = digitalRead(button1InPin);
  if (button1Val != lastButton1Val) {
    if (button1Val == HIGH) {
      if(curValueLed1 >= arrNumsLength-1){
        curValueLed1 = 0;
      }else{
        curValueLed1++;
      }
      saveConfigLed1(curValueLed1);
    } 
    delay(50);
  }
  lastButton1Val = button1Val;
  writeNumLed1(curValueLed1);
}

void lookForConfigChangesLed2() {
  int arrNumsLength = sizeof(numsArray)/sizeof(int);
  button2Val = digitalRead(button2InPin);
  if (button2Val != lastButton2Val) {
    if (button2Val == HIGH) {
      if(curValueLed2 >= arrNumsLength-1){
        curValueLed2 = 0;
      }else{
        curValueLed2++;
      }
      saveConfigLed2(curValueLed2);
    } 
    delay(50);
  }
  lastButton2Val = button2Val;
  writeNumLed2(curValueLed2);
}

void lookForManualOnOff() {
  button3Val = digitalRead(button3InPin);
  if (button3Val != lastButton3Val) {
    if (button3Val == HIGH) {
      performActionManually();
    } 
    delay(50);
  }
  lastButton3Val = button3Val;
}

void loopRadio() {
   // if there is data ready
   if ( radio.available() ){
      // Dump the payloads until we've gotten everything
      unsigned short message;
      bool done;
      // char * new;
      unsigned short rawMessage; 
      done = false;
      while ( radio.available() ){
        // Fetch the payload, and see if this was the last one.
        radio.read( &rawMessage, sizeof(unsigned long) );
        // Spew it
        printf("Got message %d...",rawMessage); 
        performAction(rawMessage);
        delay(10);
      }
   } 
}

void loop() {
  loopRadio();
  lookForConfigChangesLed1();
  lookForConfigChangesLed2(); 
  lookForManualOnOff();
}
