// Purpose:
//   This sketch will wait for input pin 4 to be pushed and then will start a relay on putput pin 5.
//   The relay will stay on until pin A0 has a state change.  The relay will then turn off for 4 seconds 
//   and start again until pin A1 is has a state change.  It will then wait for another event to trigger input pin 4.
//   Hooking a movement sensor to pin 4 will trigger a relay on pin 5 to move a winshield wiper motor
//   which moves a prop to a desired position(button on pin A0), waits, and then brings it back to the starting
//   postion (button on pin A1) until the next movement.
//   Subsitute code in functions and pin assignments as needed.

// References:
//   https://www.openhomeautomation.net/monitor-your-home-remotely-using-the-arduino-wifi-shield/
//   https://github.com/GreyGnome/EnableInterrupt/wiki/Usage#Summary

//Import this phenominal library
#include <EnableInterrupt.h>

// Pin Definitions
#define movementInPin 8 //pin to start this thing off.  Will be replaced with a motion sensor.
#define extended180InPin A0 //pin that determines when the wiper is fully extended.  For interrupt to work, use a compatible digial pin.
#define extended0InPin A1 //pin that determines when the wiper is reset to starting position.  For interrupt to work, use a compatible digial pin.
#define relay 7 //pin that the relay is connected to.

//Global variables for button interrupts
volatile int vCurValueExtended180 = 0;
volatile int vCurValueExtended0 = 0;

//Global variable which determines whether or not the prop is in motion
int inMotion = 0;

void moveProp(){
   if(inMotion == 0){
      inMotion = 1;
      //Motion detected, reset button status values and start up the wipers
      vCurValueExtended180 = 0;
      vCurValueExtended0 = 0;
      Serial.print("Waiting to reach the fully extended position (180 degrees). Starting movement. ");
      Serial.println();
      do{
         digitalWrite(relay, HIGH);
      }
      while (vCurValueExtended180 == 0);
      
      //Stop the relay and delay for a bit
      Serial.print("Fully extended position has been reached (180 degrees). Pausing movement. ");
      Serial.println();
      digitalWrite(relay, LOW);
      Serial.print("Delaying for a few seconds ");
      Serial.println();
      delay(4000);
      
      //Start the relay and return back to starting position
      Serial.print("Waiting to reach the starting position (0 degrees). Starting movement. ");
      Serial.println();
      do{
         digitalWrite(relay, HIGH);
      }
      while (vCurValueExtended0 == 0);

      //Stop the relay and and sit and wait for another movement
      Serial.print("Starting position has been reached (0 degrees). Stopping movement. ");
      Serial.println();
      digitalWrite(relay, LOW);

      Serial.print("Delaying for a few more seconds ");
      Serial.println();
      delay(8000);
      inMotion = 0;
   }     
}

void resetPropToStartingPosition() {
   Serial.print("Resetting to starting position ");
   Serial.println();
   //Start the relay and return back to starting position.
   Serial.print("Waiting to reach the starting position (0 degrees). Starting movement. ");
   Serial.println();
   do{
      digitalWrite(relay, HIGH);
   }
   while (vCurValueExtended0 == 0);
   
   //Stop the relay and and sit and wait for another movement
   Serial.print("Starting position has been reached (0 degrees). Stopping movement. ");
   Serial.println();
   digitalWrite(relay, LOW);
}

void isrMovement() {
   int movementVal;
   movementVal = digitalRead(movementInPin);
   if (movementVal == HIGH) {
      Serial.print("Movement detected ");
      Serial.println();
      moveProp();
   } 
   delay(50);
}

void isr180() {
   int extended180Val;
   extended180Val = digitalRead(extended180InPin);
   if (extended180Val == HIGH) {
     vCurValueExtended180 = 1;
     //Used for testing...
     digitalWrite(relay, HIGH);
     Serial.print("Extended ");
     Serial.println();
   } 
}

void isr0() {
  int extended0Val;
  extended0Val = digitalRead(extended0InPin);
  if (extended0Val == HIGH) {
    
     vCurValueExtended0 = 1;
     //Used for testing...
     digitalWrite(relay, LOW);
     Serial.print("Retracted ");
     Serial.println();
  } 
}

void setup() {
   //Start some debugging
   Serial.begin(9600);
   Serial.print("Setting up ");
   Serial.println();

   //Setup relay pin
   pinMode(relay, OUTPUT);
   digitalWrite(relay, LOW);

   //Setup movement detection pin
   pinMode(movementInPin, INPUT_PULLUP);
   enableInterrupt(movementInPin, isrMovement, CHANGE);

   //Setup extended to 180 (extended) pin
   pinMode(extended180InPin, INPUT_PULLUP);
   enableInterrupt(extended180InPin, isr180, CHANGE);

   //Setup extended to 0 (retracted) pin
   pinMode(extended0InPin, INPUT_PULLUP);
   enableInterrupt(extended0InPin, isr0, CHANGE);

   //Run through one full cylce to reset our selves.. Delete if you don't want it..
   resetPropToStartingPosition();
}

void loop() {

}
