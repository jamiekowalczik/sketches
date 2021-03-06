// Purpose:
//   This sketch will wait for input pin 8 to be pushed and then will start a relay on putput pin 7.
//   The relay will stay on until pin A0 has a state change.  The relay will then turn off for 5 seconds 
//   and start again until pin A1 has a state change at which point it will turn off again for 5 seconds.  
//   It will then wait for another event to trigger input pin 8.
//   Hooking a movement sensor to pin 8 will trigger a relay on pin 7 to move a winshield wiper motor
//   which moves a prop to a desired position(button on pin A0), waits, and then brings it back to the starting
//   postion (button on pin A1) until the next movement.
//   Subsitute code in functions and pin assignments as needed.

// References:
//   https://www.openhomeautomation.net/monitor-your-home-remotely-using-the-arduino-wifi-shield/
//   https://github.com/GreyGnome/EnableInterrupt/wiki/Usage#Summary
//   --Library Download: https://bintray.com/greygnome/generic/EnableInterrupt/view#files

//Import this phenominal library
#include <EnableInterrupt.h>

// Pin Definitions
#define movementInPin 8 //pin to start this thing off.  Will be replaced with a motion sensor.
#define relayOutPin 7 //pin that the relay is connected to.
#define extended180InPin A0 //pin that determines when the wiper is fully extended.  Use EnableInterrupt Library.
#define extended0InPin A1 //pin that determines when the wiper is reset to starting position.  Use EnableInterrupt Library.

//Global variables for button interrupts
volatile int vCurValueExtended180 = 0;
volatile int vCurValueExtended0 = 0;

//Global variable which determines whether or not the prop is in motion
volatile int vInMotion = 0;

void moveProp(){
   if(vInMotion == 0){
      vInMotion = 1;
      //Motion detected, reset button status values and start up the wipers
      vCurValueExtended180 = 0;
      vCurValueExtended0 = 0;
      Serial.println("Waiting to reach the fully extended position (180 degrees). Starting movement. ");
      do{
         digitalWrite(relayOutPin, HIGH);
      }
      while (vCurValueExtended180 == 0);
      
      //Stop the relay and delay for a bit
      Serial.println("Fully extended position has been reached (180 degrees). Pausing movement. ");
      digitalWrite(relayOutPin, LOW);
      Serial.println("Delaying for a few seconds ");
      delay(5000);
      
      //Start the relay and return back to starting position
      Serial.println("Waiting to reach the starting position (0 degrees). Starting movement. ");
      do{
         digitalWrite(relayOutPin, HIGH);
      }
      while (vCurValueExtended0 == 0);

      //Stop the relay and and sit and wait for another movement
      Serial.println("Starting position has been reached (0 degrees). Stopping movement. ");
      digitalWrite(relayOutPin, LOW);

      Serial.println("Delaying for a few more seconds ");
      delay(5000);
      vInMotion = 0;
   }     
}

void resetPropToStartingPosition() {
   Serial.println("Resetting to starting position ");
   //Start the relay and return back to starting position.
   Serial.println("Waiting to reach the starting position (0 degrees). Starting movement. ");
   vInMotion = 1;
   do{
      digitalWrite(relayOutPin, HIGH);
   }
   while (vCurValueExtended0 == 0);
   
   //Stop the relay and and sit and wait for another movement
   Serial.println("Starting position has been reached (0 degrees). Stopping movement. ");
   digitalWrite(relayOutPin, LOW);
   vInMotion = 0;
}

void isrMovement() {
   Serial.println("Movement detected ");
   moveProp();
}

void isr180() {
   //Used for testing.  Remove digitalWrite line for production...
   digitalWrite(relayOutPin, HIGH);
   Serial.println("Extended ");
   vCurValueExtended180 = 1;
}

void isr0() {
   //Used for testing.  Remove digitalWrite line for production...
   digitalWrite(relayOutPin, LOW);
   Serial.println("Retracted ");
   vCurValueExtended0 = 1;
}

void setup() {
   //Start some debugging
   Serial.begin(9600);
   Serial.println("");
   Serial.println("----------");
   Serial.println("Setting up ");

   //Setup relay pin
   pinMode(relayOutPin, OUTPUT);
   digitalWrite(relayOutPin, LOW);

   //Setup movement detection pin
   pinMode(movementInPin, INPUT_PULLUP);
   enableInterrupt(movementInPin, isrMovement, RISING);

   //Setup extended to 180 (extended) pin
   pinMode(extended180InPin, INPUT_PULLUP);
   enableInterrupt(extended180InPin, isr180, RISING);

   //Setup extended to 0 (retracted) pin
   pinMode(extended0InPin, INPUT_PULLUP);
   enableInterrupt(extended0InPin, isr0, RISING);

   //Run through one full cylce to reset our selves.. Delete if you don't want it..
   resetPropToStartingPosition();
}

void loop() { 
}
