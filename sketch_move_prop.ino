// Purpose:
//   This sketch will wait for input pin 4 to be pushed and then will start a relay on putput pin 5.
//   The relay will stay on until input pin 2 is pushed.  The relay will then turn off for 4 seconds 
//   and start again until input pin 3 is pushed.  It will then wait for another event to trigger input pin 4.
//   Hooking a movement sensor to pin 4 will trigger a relay on pin 5 to move a winshield wiper motor
//   which moves a prop to a desired position(button on pin 2), waits, and then brings it back to the starting
//   postion (button on pin 3) until the next movement.
//   Subsitute code in functions and pin assignments as needed.

// References:
//   https://www.openhomeautomation.net/monitor-your-home-remotely-using-the-arduino-wifi-shield/
//   http://www.geertlangereis.nl/Electronics/Pin_Change_Interrupts/PinChange_en.html

int movementInPin = 4; //pin to start this thing off.  Will be replaced with a motion sensor
int movementVal = 1;
int lastMovementVal = 1;

int extended180InPin = 2; //pin that determines when the wiper is fully extended.  For interrupt to work, use a compatible digial pin.
int extended180Val = 1;
volatile int vLastExtended180Val = 1;
volatile int vCurValueExtended180 = 0;

int extended0InPin = 3; //pin that determines when the wiper is reset to starting position.  For interrupt to work, use a compatible digial pin.
int extended0Val = 1;
volatile int vLastExtended0Val = 1;
volatile int vCurValueExtended0 = 0;

int relay = 5; //pin that controls the relay to move the prop

int inMotion = 0;

void lookForMotionLoop() {
  movementVal = digitalRead(movementInPin);
  if (movementVal != lastMovementVal) {
    if (movementVal == HIGH) {
       moveProp();
    } 
    delay(50);
  }
  lastMovementVal = movementVal;
}

void moveProp(){
   if(inMotion == 0){
      inMotion = 1;
      //Motion detected, reset button status values
      //and start up the wipers
      vCurValueExtended180 = 0;
      vCurValueExtended0 = 0;
      do{
         digitalWrite(relay, HIGH);
      }
      while (vCurValueExtended180 == 0);
      
      //Stop the relay and delay for a bit
      digitalWrite(relay, LOW);
      delay(4000);
      
      //Start the relay and return back to starting position
      do{
         digitalWrite(relay, HIGH);
      }
      while (vCurValueExtended0 == 0);

      //Stop the relay and and sit and wait for another movement
      digitalWrite(relay, LOW);
      inMotion = 0;
   }     
}

void resetPropToStartingPosition() {
   //Start the relay and return back to starting position
   do{
      digitalWrite(relay, HIGH);
   }
   while (vCurValueExtended0 == 0);

   //Stop the relay and and sit and wait for another movement
   digitalWrite(relay, LOW);
}

void isr180() {
  extended180Val = digitalRead(extended180InPin);
  if (extended180Val != vLastExtended180Val) {
    if (extended180Val == HIGH) {
      vCurValueExtended180 = 1;
    } 
  }
  vLastExtended180Val = extended180Val;
}

void isr0() {
  extended0Val = digitalRead(extended0InPin);
  if (extended0Val != vLastExtended0Val) {
    if (extended0Val == HIGH) {
      vCurValueExtended0 = 1;
    } 
  }
  vLastExtended0Val = extended0Val;
}

void setup() {
   pinMode(movementInPin, INPUT);
   pinMode(extended180InPin, INPUT);
   pinMode(extended0InPin, INPUT);
  
   pinMode(relay, OUTPUT);
   digitalWrite(relay, LOW);
   
   //Use a compatible digital pin to use interrupts. UNO 0=pin2, 1=pin3.  Or use digitalPinToInterrupt function.
   attachInterrupt(digitalPinToInterrupt(extended180InPin), isr180, CHANGE);
   attachInterrupt(digitalPinToInterrupt(extended0InPin), isr0, CHANGE);
   //resetPropToStartingPosition();
}

void loop() {
  lookForMotionLoop();
}
