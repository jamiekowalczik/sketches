//Import this phenominal library
//#include <EnableInterrupt.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <stdio.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <menu.h>
#include <genericKeyboard.h>
#include <chainStream.h>
#include <menuLCDs.h>
#include <menuFields.h>
#include <controls/common/menuTextFields.h>
#include <Time.h>
#include "RTClib.h"
#include "dht.h"
#include <Thread.h>
#include <EEPROMex.h>

// Pin Definitions
#define pinEnter A3
#define keyboard_AnalogInput 3
#define btnRIGHT  menu::enterCode
#define btnUP     menu::downCode
#define btnDOWN   menu::upCode
#define btnLEFT   menu::escCode
#define btnENTER  menu::enterCode
#define btnNONE   -1
#define depthHighPin A2
#define depthHighID 2
#define ONE_WIRE_BUS 4
#define DHTPIN 3

byte DHTTYPE = 0;

//Variables containing data we want to keep persistent
byte lastConfigID;
byte ConfigID;

char charWaterTemp[20];
byte bWaterTemp, bOldWaterTemp;

byte xeeConfigID;
byte addrConfigID = 10;
byte xeeRelay1;
byte addrRelay1 = 20;
int relay1 = 5;
int relay1Val = 0;
int relay1OldVal = 0;
int relay2 = 2;
int relay2Val = 0;
int relay2OldVal = 0;
byte xeeRelay2;
byte addrRelay2 = 30;
byte xeeDHTType = 0, xeeLastDHTType = 0;
byte addrDHTType = 20;
char charRoomTemp[20];
char charRoomHumidity[20];
byte bRoomTemp, bOldRoomTemp, bRoomHumidity, bOldRoomHumidity;
float fRoomTemp, fRoomHumidity;

genericKeyboard mykb(read_keyboard);
//alternative to previous but now we can input from Serial too...
Stream* in2[]={&mykb,&Serial};
chainStream<2> allIn(in2);

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
menuLCD menu_lcd(lcd,20,4);//menu output device

void nothing() {}

RTC_DS1307 rtc;    // Create a RealTimeClock object
OneWire ourWire(ONE_WIRE_BUS);
DallasTemperature sensors(&ourWire);

Thread t_LCDUpdate = Thread();
Thread t_readRF24 = Thread();
Thread t_WaterSensorTemp = Thread();
Thread t_WaterSensorDepth = Thread();
Thread t_AirSensorTempHumidity = Thread();
Thread t_Timestamp = Thread();

////// MENU

TOGGLE(xeeDHTType,dhtTypes,"DHT Types: ",
    VALUE("DHT11",0, toggleDHTType),
    VALUE("DHT22",1, toggleDHTType)
);

TOGGLE(relay1Val,relay1Menu,"Relay 1: ",
    VALUE("On",1, toggleRelay1),
    VALUE("Off",0, toggleRelay1)
);

TOGGLE(relay2Val,relay2Menu,"Relay 2: ",
    VALUE("On",1, toggleRelay2),
    VALUE("Off",0, toggleRelay2)
);

byte bRetVal;
//field value, click to browse, click to choose
CHOOSE(bRetVal,some_sample,"Choice: ",
    VALUE("A",0,nothing),
    VALUE("B",1,nothing),
    VALUE("C",2,nothing)
);

MENU(subMenuSensorData,"Sensor Data",
   OP("Water Level: ",nothing),
   OP("Water Temp: ",nothing),
   OP("Room Temp: ",nothing),
   OP("Humidity: ",nothing)
);

MENU(actionMenu,"Action",
   OP("Fill",fillCmd),
   OP("Empty",emptyCmd),
   SUBMENU(relay1Menu),
   SUBMENU(relay2Menu),
   SUBMENU(some_sample)
);

byte bHour, bMinute, bDay, bMonth;
int iYear;
MENU(setDateTimeMenu,"Set Date/Time",
   FIELD(bHour,"Hour","",0,23,-10,-1,updateRTC),
   FIELD(bMinute,"Minute","",0,59,-10,-1,updateRTC),
   FIELD(bMonth,"Month","",1,12,-10,-1,updateRTC),
   FIELD(bDay,"Day","",1,31,-10,-1,updateRTC),
   FIELD(iYear,"Year","",2015,3015,-10,-1,updateRTC)
);

MENU(setupMenu,"Setup",
   FIELD(ConfigID,"Unit ID","",0,99,-10,-1,updateConfigID),
   SUBMENU(setDateTimeMenu),
   SUBMENU(dhtTypes)
);
  
MENU(mainMenu,"Main",
  OP("",nothing),
  SUBMENU(subMenuSensorData),
  SUBMENU(actionMenu),
  SUBMENU(setupMenu)
);

void toggleRelay1(){
  Serial.println(relay1Val);
  if(relay1Val == 0){
    relay1OldVal = 0;
    saveConfigRelay1(0);
    digitalWrite(relay1, LOW);
  }else{
    relay1OldVal = 1;
    saveConfigRelay1(1);
    digitalWrite(relay1, HIGH);
  }
}

void toggleRelay2(){
  if(relay2Val == 0){
    relay2OldVal = 0;
    saveConfigRelay2(0);
    digitalWrite(relay2, LOW);
  }else{
    relay2OldVal = 1;
    saveConfigRelay2(1);
    digitalWrite(relay2, HIGH);
  }
}

void saveConfigID(byte i) {
  xeeConfigID = byte(i);
  EEPROM.write(addrConfigID, xeeConfigID);
}

void saveConfigRelay1(byte i) {
  xeeRelay1 = byte(i);
  EEPROM.write(addrRelay1,xeeRelay1);
}

void saveConfigRelay2(byte i) {
  xeeRelay2 = byte(i);
  EEPROM.write(addrRelay2,xeeRelay2);
}

void saveDHTType(byte i){
  xeeDHTType = byte(i);
  EEPROM.write(addrDHTType, xeeDHTType);
}


dht DHT;
void readConfig() {
  xeeConfigID = EEPROM.read(addrConfigID);
  ConfigID = byte(xeeConfigID);
  if (ConfigID < 0 || ConfigID >99){
     ConfigID = 0;
  }
  lastConfigID = ConfigID;

  xeeRelay1 = EEPROM.read(addrRelay1);
  relay1Val = byte(xeeRelay1);
  if (relay1Val == 0) {
     digitalWrite(relay1, LOW);
  }else if(relay1Val == 1) {
     digitalWrite(relay1, HIGH);
  }

  xeeRelay2 = EEPROM.read(addrRelay2);
  relay2Val = byte(xeeRelay2);
  if (relay2Val == 0) {
     digitalWrite(relay2, LOW);
  }else if(relay2Val == 1) {
     digitalWrite(relay2, HIGH);
  }

  if(xeeDHTType == 0){
    xeeLastDHTType = 0;
  }
}

void updateConfigID() {
  if (ConfigID != lastConfigID) {
    saveConfigID(ConfigID);
    lastConfigID = ConfigID;
  }
}

void toggleDHTType() {
  if (xeeDHTType != xeeLastDHTType) {
    saveDHTType(xeeDHTType);
    xeeLastDHTType = xeeDHTType;
  }
}

int ISHIGH = 0;

void emptyCmd() {
  lcd.clear();
  delay(1000);
  readWaterDepth();
  if(ISHIGH == 0){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Reservoir Is Empty  "));
  }else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Emptying Reservoir  "));
    relay1Val = 1;
    saveConfigRelay1(1);
    digitalWrite(relay1, HIGH);
  }
    
  int curStatus = 0;
  int dots = 0;
  do{
    if(curStatus == 0){
      UpdateEmptyStatus(curStatus);
      curStatus = 1;
    }else{
      UpdateEmptyStatus(curStatus);
      curStatus = 0;
    }

    lcd.setCursor(dots,2);
    if(dots < 20){
      lcd.print(F("."));
      dots++;
    }else{
      dots = 0;
      lcd.setCursor(0,2);
      lcd.print("                    ");
    }
    delay(1000);
    readWaterDepth();
  } while (ISHIGH == 1);

  lcd.clear();
  lcd.setCursor(0,3);
  lcd.print(F(" ****Completed****  "));

  relay1Val = 0;
  saveConfigRelay1(0);
  digitalWrite(relay1, LOW);
  delay(2000);
  mainMenu.focus(1);
  mainMenu.redraw(menu_lcd,allIn); 
}

void UpdateEmptyStatus(byte iStatus){
  String EmptyStatus[2] = {"Drain Valve Is Open ", "Running Drain Pump  "};
  lcd.setCursor(0,1);
  lcd.print(EmptyStatus[iStatus]);
}

void fillCmd() {
  lcd.clear();
  delay(1000);
  readWaterDepth();
  if(ISHIGH == 1){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Reservoir Is Full   "));
  }else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Filling Reservoir   "));
    relay2Val = 1;
    saveConfigRelay2(1);
    digitalWrite(relay2, HIGH);
  }
    
  byte curStatus = 0, dots = 0;
  do{
    if(curStatus == 0){
      UpdateFillStatus(curStatus);
      curStatus = 1;
    }else{
      UpdateFillStatus(curStatus);
      curStatus = 0;
    }

    lcd.setCursor(dots,2);
    if(dots < 20){
      lcd.print(F("."));
      dots++;
    }else{
      dots = 0;
      lcd.setCursor(0,2);
      lcd.print("                    ");
    }
    delay(1000);
    readWaterDepth();
  } while (ISHIGH == 0);

  lcd.clear();
  lcd.setCursor(0,3);
  lcd.print(F(" ****Completed****  "));

  relay2Val = 0;
  saveConfigRelay2(0);
  digitalWrite(relay2, LOW);
  delay(2000);
  mainMenu.focus(1);
  mainMenu.redraw(menu_lcd,allIn); 
}

void UpdateFillStatus(byte iStatus){
  String FillStatus[2] = {"Fill Valve Is Open  ", "Running Fill Pump   "};
  lcd.setCursor(0,1);
  lcd.print(FillStatus[iStatus]);
}

byte updateWaterTempAlarm() {
  return 1;
}

void updateRTC(){
  setTime(bHour,bMinute,0,bDay,bMonth,iYear);
  rtc.adjust(DateTime(now()));
  showTime();
}

RF24 radio(40,53);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void sendCallback(unsigned short callback){
   // First, stop listening so we can talk
   radio.stopListening();

   Serial.println(callback);
   
   // Send the final one back.
   radio.write( &callback, sizeof(unsigned short) );
   printf("Sent response.\n\r");

   // Now, resume listening so we catch the next packets.
   radio.startListening();
}

int old_button = 0;

int read_keyboard() {
  int button, button2, pressed_button;  
  button = getButton();
  if (button != old_button) {
      delay(50);        // debounce
      button2 = getButton();

      if (button == button2) {
         old_button = button;
         pressed_button = button;
         if(button != 0) {
           //Serial.println(button); 
           if(button == 1) return btnLEFT;
           if(button == 2) return btnUP;
           if(button == 3) return btnDOWN;
           if(button == 4) return btnRIGHT;
           if(button == 5) return btnENTER;
         }
      }
  }else{
    return btnNONE;
  }
}

int getButton() {
  int i, z, button;
  int sum = 0;

  for (i=0; i < 4; i++) {
     sum += analogRead(keyboard_AnalogInput);
  }
  z = sum / 4;
  if (z > 1000) button = 0;                                           
  else if (z >= 0 && z < 20) button = 1; //LEFT                    
  else if (z > 135 && z < 155) button = 2; //UP                
  else if (z > 315 && z < 335) button = 3; //DOWN                
  else if (z > 495 && z < 515) button = 4; //RIGHT           
  else if (z > 725 && z < 745) button = 5; //ENTER
  else button = 0;

  return button;
}

void performAction(unsigned short rawMessage){
   unsigned short action, piid, deviceid, callback;
   Serial.println(rawMessage);
   //int input_number = int(rawMessage);
   //action = (input_number%10);
   //deviceid = ((input_number/10)%10);
   //piid = ((input_number/100)%10);
   
   if(rawMessage == 100){
    callback=100;
    sendCallback(callback);
    fillCmd();
   }else if(rawMessage == 101){
    callback=101;
    sendCallback(callback);
    emptyCmd();
   }else if(rawMessage == 83){
     saveConfigRelay1(1);
     digitalWrite(relay1, HIGH);
     sendCallback(0);
   }else if(rawMessage == 84){
     saveConfigRelay2(1);
     digitalWrite(relay2, HIGH);
     sendCallback(0);
   }else if(rawMessage == 85){
     saveConfigRelay1(0);
     digitalWrite(relay1, LOW);
     sendCallback(0);
   }else if(rawMessage == 86){
     saveConfigRelay2(0);
     digitalWrite(relay2, LOW);
     sendCallback(0);
   }else if(rawMessage == 87){
    // WATER TEMP //
    radio.stopListening();
    readSensorTemperature();
    readRoomTemperatureAndHumidity();
    callback=(short)bWaterTemp;
    sendCallback(callback);
   }else if(rawMessage == 88){
    // ROOM TEMP //
    radio.stopListening();
    readRoomTemperatureAndHumidity();
    callback=(short)bRoomTemp;
    sendCallback(callback);
   }else if(rawMessage == 89){
    // ROOM HUMIDITY //
    radio.stopListening();
    readRoomTemperatureAndHumidity();
    callback=(short)bRoomHumidity;
    sendCallback(callback);
   }else if(rawMessage == 90){
    // WATER LEVEL //
    radio.stopListening();
    readWaterDepth();
    callback=(short)ISHIGH;
    sendCallback(callback);
   }else{
     callback=999;
     sendCallback(callback);
   }
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
        Serial.println("Message"+(String)rawMessage); 
        performAction(rawMessage);
        delay(10);
      }
   }
}

void readSensorTemperature() {
  sensors.requestTemperatures();
  bWaterTemp = sensors.getTempFByIndex(0);
  String strWaterTempPre = "Water Temp: ", strWaterTemp, strJoinedString;
  byte str_len;
  if(bOldWaterTemp != bWaterTemp && bWaterTemp != 185 && bWaterTemp != -196) {
    strWaterTemp = String(bWaterTemp);
    strJoinedString = strWaterTempPre+strWaterTemp+" F";
    str_len = strJoinedString.length() + 1;
    charWaterTemp[str_len];
    strJoinedString.toCharArray(charWaterTemp, str_len);
    subMenuSensorData.data[1]->text = charWaterTemp;
    bOldWaterTemp = bWaterTemp;
    subMenuSensorData.redraw(menu_lcd,allIn);
  }else if(bOldWaterTemp != bWaterTemp && (bWaterTemp == 185 || bWaterTemp == -196)){
    strWaterTemp = "Error";
    strJoinedString = strWaterTempPre+strWaterTemp;
    str_len = strJoinedString.length() + 1;
    charWaterTemp[str_len];
    strJoinedString.toCharArray(charWaterTemp, str_len);
    subMenuSensorData.data[1]->text = charWaterTemp;
    bOldWaterTemp = bWaterTemp;
    subMenuSensorData.redraw(menu_lcd,allIn);
  }
}

String curWaterDepthVal, lastWaterDepthVal;
char charWaterLevel[20];
void readWaterDepth() {
  int curVal = analogRead(depthHighID);
  if(curVal > 425 && curVal < 750){
    curWaterDepthVal = "High";
    ISHIGH = 1;
  }else if(curVal > 750){
    curWaterDepthVal = "Error";
    ISHIGH = 1;
  }else{
    curWaterDepthVal = "Low"; 
    ISHIGH = 0;
  }
  if(lastWaterDepthVal != curWaterDepthVal) {
    String strWaterLevelPre = "Water Level: ";
    String strWaterLevel = String(curWaterDepthVal);
    String strJoinedString = strWaterLevelPre+strWaterLevel;
    byte str_len = strJoinedString.length() + 1;
    charWaterLevel[str_len];
    strJoinedString.toCharArray(charWaterLevel, str_len);
    subMenuSensorData.data[0]->text = charWaterLevel;
    lastWaterDepthVal = curWaterDepthVal;
    subMenuSensorData.redraw(menu_lcd,allIn);
  }
}

double Fahrenheit(double celsius){
  return 1.8 * celsius + 32;
}

void readRoomTemperatureAndHumidity() {
  //float fRoomHumidity = dht.readHumidity();
  DHT.read11(3);
  float fRoomHumidity = DHT.humidity;
  //float fRoomTemp = dht.readTemperature(true);
  float cRoomTemp = DHT.temperature;
  float fRoomTemp = Fahrenheit(cRoomTemp);
  String strRoomTempPre = "Room Temp: ";
  String strRoomHumidityPre = "Humidity: ";
  String strRoomHumidity;
  String strRoomTemp, strJoinedString;
  byte str_len;
  
  bRoomTemp = (int) fRoomTemp;
  bRoomHumidity = (int) fRoomHumidity;

  if (isnan(fRoomHumidity) || isnan(fRoomTemp)) {
    bRoomTemp = 0;
    if(bRoomTemp != bOldRoomTemp) {
      strRoomTemp = "Error";
      strJoinedString = strRoomTempPre+strRoomTemp;
      str_len = strJoinedString.length() + 1;
      charRoomTemp[str_len];
      strJoinedString.toCharArray(charRoomTemp, str_len);
      subMenuSensorData.data[2]->text = charRoomTemp;
      bOldRoomTemp = bRoomTemp;
      subMenuSensorData.redraw(menu_lcd,allIn);
    }
    bRoomHumidity = 0;
    if(bOldRoomHumidity != bRoomHumidity) {
      strRoomHumidity = "Error";
      strJoinedString = strRoomHumidityPre+strRoomHumidity;
      str_len = strJoinedString.length() + 1;
      charRoomHumidity[str_len];
      strJoinedString.toCharArray(charRoomHumidity, str_len);
      subMenuSensorData.data[3]->text = charRoomHumidity;
      bOldRoomHumidity = bRoomHumidity;
      subMenuSensorData.redraw(menu_lcd,allIn);
    }
    return;
  }

  if(bRoomTemp != bOldRoomTemp) {
    strRoomTempPre = "Room Temp: ";
    strRoomTemp = String(bRoomTemp);
    strJoinedString = strRoomTempPre+strRoomTemp+" F";
    str_len = strJoinedString.length() + 1;
    charRoomTemp[str_len];
    strJoinedString.toCharArray(charRoomTemp, str_len);
    subMenuSensorData.data[2]->text = charRoomTemp;
    bOldRoomTemp = bRoomTemp;
    subMenuSensorData.redraw(menu_lcd,allIn);
  }

  if(bOldRoomHumidity != bRoomHumidity) {
    strRoomHumidityPre = "Humidity: ";
    strRoomHumidity = String(bRoomHumidity);
    strJoinedString = strRoomHumidityPre+strRoomHumidity+" %";
    str_len = strJoinedString.length() + 1;
    charRoomHumidity[str_len];
    strJoinedString.toCharArray(charRoomHumidity, str_len);
    subMenuSensorData.data[3]->text = charRoomHumidity;
    bOldRoomHumidity = bRoomHumidity;
    subMenuSensorData.redraw(menu_lcd,allIn);
  }
}

char cDateTime[20], cHourString[20];
byte bCurMonth, bOldMonth, bCurDay, bOldDay, bCurHour, bOldHour, bCurMinute, bOldMinute;
int iCurYear, iOldYear;
void showTime() {
  DateTime now = rtc.now();  // Read data from the RTC Chip
  iCurYear = now.year();
  bCurMonth = now.month();
  bCurDay = now.day();
  bCurHour = now.hour();
  bCurMinute = now.minute();
  if(iCurYear != iOldYear || bCurMonth != bOldMonth || bCurDay != bOldDay || bCurHour != bOldHour || bCurMinute != bOldMinute){
    iOldYear = iCurYear;
    bOldMonth = bCurMonth;
    bOldDay = bCurDay;
    bOldHour = bCurHour;
    bOldMinute = bCurMinute;

    iYear = iCurYear;
    bMonth = bCurMonth;
    bDay = bCurDay;
    bHour = bCurHour;
    bMinute = bCurMinute;
    
    String AMPM = " AM";
    if(bCurHour > 12){
      bCurHour = bCurHour - 12;
      AMPM = " PM";
    }else if(bCurHour == 0){
      bCurHour = 12;
    }else if(bCurHour == 12){
      AMPM = " PM";
    }

    String sCurMinute;
    if(bCurMinute < 10){
      sCurMinute = String(bCurMinute);
      sCurMinute = "0"+sCurMinute;
    }else{
      sCurMinute = String(bCurMinute);
    }
    
    String strTime = String(bCurHour)+":"+sCurMinute+AMPM+" "+bCurMonth+"/"+bCurDay+"/"+iCurYear;
    String strJoinedString = strTime;
    byte str_len = strJoinedString.length() + 1;
    cDateTime[str_len];
    strJoinedString.toCharArray(cDateTime, str_len);

    mainMenu.data[0]->text = cDateTime;
    mainMenu.redraw(menu_lcd,allIn); 
  }
}

void setup() {
  //delay(1000);
  Serial.begin(9600);
    
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  
  readConfig();
  
  Wire.begin();
  lcd.begin(20,4);
  lcd.print("Ok"); 

  sensors.begin();
  
  rtc.begin(); // Start the RTC library code

  t_LCDUpdate.onRun(updateLCD);
  t_LCDUpdate.setInterval(50);

  t_readRF24.onRun(loopRadio);
  t_readRF24.setInterval(50);
  
  t_WaterSensorTemp.onRun(readSensorTemperature);
  t_WaterSensorTemp.setInterval(3000);

  t_WaterSensorDepth.onRun(readWaterDepth);
  t_WaterSensorDepth.setInterval(1500);

  t_AirSensorTempHumidity.onRun(readRoomTemperatureAndHumidity);
  t_AirSensorTempHumidity.setInterval(3000);

  t_Timestamp.onRun(showTime);
  t_Timestamp.setInterval(500);
  
  radio.begin();
  radio.setAutoAck(1); // Ensure autoACK is enabled
  radio.setRetries(15,15);

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
  radio.printDetails();
}

void updateLCD(){
  mainMenu.poll(menu_lcd,allIn);
}

String curMenu, prevMenu;
void loop() {
  if(t_LCDUpdate.shouldRun())
      t_LCDUpdate.run();
  if(t_readRF24.shouldRun())
      t_readRF24.run();
  curMenu = ((menu*)menuNode::activeNode)->text;
  if (curMenu == "Sensor Data") {
    if(t_WaterSensorTemp.shouldRun())
      t_WaterSensorTemp.run();
    if(t_WaterSensorDepth.shouldRun())
      t_WaterSensorDepth.run();
    if(t_AirSensorTempHumidity.shouldRun())
      t_AirSensorTempHumidity.run();
  }else if(curMenu == "Main"){
    if(t_Timestamp.shouldRun())
      t_Timestamp.run();
  }
}
