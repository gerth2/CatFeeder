#include <Servo.h>
#include "HD44780_LCD_PCF8574.h"
#include <Wire.h>
#include <I2C_RTC.h>

Servo feedServo;
#define FEED_SERVO_PIN 10
int pos = 0;

#define SERVO_FEED_SPEED 120
#define SERVO_STOP_SPEED 90

#define FEED_CYCLE_SEC 8
#define FEED_TIME_HR 9
#define FEED_TIME_MIN 25

#define FEED_NOW_BTN_PIN 9
bool curFeedNow = false;
bool prevFeedNow = false;

#define LOOP_TIME_SEC 0.1

int lastFedYear = 2023;
int lastFedMonth = 1;
int lastFedDate = 1;
int lastFedHour = 0;
int lastFedMin = 0;
int lastFedSec = 0;



bool prevIsFeedTime = false;
bool curIsFeedTime = false;

double feedTimerSec = 0;


HD44780LCD myLCD(4, 20, 0x27, &Wire);
static DS3231 RTC;

void setup() {
  feedServo.attach(FEED_SERVO_PIN);

  myLCD.PCF8574_LCDInit(myLCD.LCDCursorTypeOff);
  myLCD.PCF8574_LCDClearScreen();
  myLCD.PCF8574_LCDBackLightSet(true);
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberOne, 0);

  RTC.begin();
  RTC.setHourMode(CLOCK_H24);

  //RTC.setTime(7,7,0);
  //RTC.setDate(4,7,2023);

  pinMode(FEED_NOW_BTN_PIN, INPUT_PULLUP);

}

void startFeed(){
  feedTimerSec = FEED_CYCLE_SEC;
  lastFedYear = RTC.getYear();
  lastFedMonth = RTC.getMonth();
  lastFedDate = RTC.getDay();
  lastFedHour = RTC.getHours();
  lastFedMin = RTC.getMinutes();
  lastFedSec = RTC.getSeconds();
}

bool isFeedTime(){
  prevIsFeedTime = curIsFeedTime;
  curIsFeedTime = RTC.getHours() == FEED_TIME_HR && RTC.getMinutes() == FEED_TIME_MIN;
  return curIsFeedTime && ! prevIsFeedTime; // return rising edge
}

String formatDateTime(int year, int month, int date, int hr, int min, int sec){
  String retVal = "";

  retVal += String(month);
  retVal += "/";
  retVal += String(date);
  retVal += "/";
  retVal += String(year);
  retVal += " ";

  if(hr <= 9){
    retVal += " ";
  }
  if(hr <= 12){
    retVal += String(hr);
  } else {
    retVal += String(hr-12);
  }


  retVal += String(":");

  if(min <= 9){
    retVal += "0";
  }
  retVal += String(min);

  retVal += String(":");

  if(sec <= 9){
    retVal += "0";
  }

  retVal += String(sec);

  if(hr >= 12)
    retVal += String("PM");
  else
    retVal += String("AM");     

  return retVal;
}

void loop() {
  //Handle timer-based feed
  if(isFeedTime()){
    startFeed();
  }

  //Handle feed-now button
  prevFeedNow = curFeedNow;
  curFeedNow = digitalRead(FEED_NOW_BTN_PIN);
  if(curFeedNow == false && prevFeedNow == true){
    //falling edge
    startFeed();
  }

  // 90 = stop, 0 = full reverse, 180 = full fwd... ish
  if(feedTimerSec > 0.0){
    feedServo.write(SERVO_FEED_SPEED);
  } else {
    feedServo.write(SERVO_STOP_SPEED);
  }

  //update display
  //myLCD.PCF8574_LCDClearScreen();
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberOne, 0);  
  String timeStr = formatDateTime(RTC.getYear(), RTC.getMonth(), RTC.getDay(), RTC.getHours(), RTC.getMinutes(), RTC.getSeconds());
  myLCD.PCF8574_LCDSendString(timeStr.c_str());
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberTwo, 0);  
  if(feedTimerSec <= 0.0){
    myLCD.PCF8574_LCDSendString("It's not food time. ");
  } else {
    myLCD.PCF8574_LCDSendString("Feeding... ");
    double pctFed = (FEED_CYCLE_SEC - feedTimerSec) / FEED_CYCLE_SEC * 100.0;
    myLCD.PCF8574_LCDSendString(String(pctFed, 0).c_str());
    myLCD.PCF8574_LCDSendString(" %      ");
  }
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberThree, 0);  
  myLCD.PCF8574_LCDSendString("Last Fed:");
  myLCD.PCF8574_LCDGOTO(myLCD.LCDLineNumberFour, 0);  
  timeStr = formatDateTime(lastFedYear, lastFedMonth, lastFedDate, lastFedHour, lastFedMin, lastFedSec);
  myLCD.PCF8574_LCDSendString(timeStr.c_str());





  // countdown feed time
  if(feedTimerSec > 0.0){
    feedTimerSec -= LOOP_TIME_SEC;
  }

  delay(LOOP_TIME_SEC*1000);

}
