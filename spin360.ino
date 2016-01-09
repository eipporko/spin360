/*
 *
 * SPIN360
 *
 * Copyright (c) David Antunez Gonzalez 2016 <dantunezglez@gmail.com>
 *
 * All rights reserved.
 *
 * SPIN360 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library.
 *
 */

#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <math.h>

#define STEPS 200
#define DELAY_CAMERA 1000
#define MOTOR_SPEED 60

//Pins
const int encAPin = 2;
const int encBPin = 3;
const int coilAPin = 4;
const int coilBPin = 9;
const int buttonPin = 10;
const int relePin = 11;

int buttonState = LOW;
int lastButtonState = LOW;
volatile int numOfPics = 0;

struct Encoder {
  volatile int sigAVal;
  volatile int sigBVal;
  volatile int* ptrVar = NULL;
  int lowerVal;
  int higherVal;
} encoder_t;

Encoder enc;

typedef enum {NA, MENU, SETTINGS, PROGRESSION} state;
state stateProgram = NA;

Stepper platform(STEPS,5,6,7,8);
LiquidCrystal_I2C lcd(0x27,20,4);

void setup() {
  //init serial
  Serial.begin(9600);

  //init lcd
  lcd.init();
  lcd.backlight();

  //setting pins
  pinMode(encAPin, OUTPUT);
  pinMode(encBPin, OUTPUT);
  pinMode(coilAPin, OUTPUT);
  pinMode(coilBPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(relePin, OUTPUT);

  //init outputs
  digitalWrite(relePin, LOW);
  digitalWrite(coilAPin, LOW);
  digitalWrite(coilBPin, LOW);

  //init encoder variables
  enc.sigAVal = digitalRead(encAPin);
  enc.sigBVal = digitalRead(encBPin);

  attachInterrupt(digitalPinToInterrupt(encAPin), interruptEncAVal, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encBPin), interruptEncBVal, CHANGE);

  //setting speed platform
  platform.setSpeed(MOTOR_SPEED);
}


void interruptEncAVal() {
  enc.sigAVal = digitalRead(encAPin);

  if (enc.ptrVar) {
    if (enc.sigAVal != enc.sigBVal)
      //clockwise
      *enc.ptrVar = (*enc.ptrVar<enc.higherVal)?*enc.ptrVar++:enc.higherVal;
    else
      //counterclockwise
      *enc.ptrVar = (*enc.ptrVar>enc.lowerVal)?*enc.ptrVar--:enc.lowerVal;
  }
}


void interruptEncBVal() {
  enc.sigBVal = digitalRead(encBPin);

  if (enc.ptrVar) {
    if (enc.sigBVal != enc.sigAVal)
      //counterclockwise
      *enc.ptrVar = (*enc.ptrVar>enc.lowerVal)?*enc.ptrVar--:enc.lowerVal;
    else
      //clockwise
      *enc.ptrVar = (*enc.ptrVar<enc.higherVal)?*enc.ptrVar++:enc.higherVal;
  }
}


void encoderPtrVal(volatile int* ptr, int low, int high) {
  enc.lowerVal = low;
  enc.higherVal = high;
  enc.ptrVar = ptr;
}


bool checkButton() {
  lastButtonState = buttonState;
  buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH && lastButtonState == LOW)
    return true;
  else
    return false;
}


void mainMenu() {

  //print main menu
  if (stateProgram != MENU) {
    stateProgram = MENU;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MAIN MENU");
    lcd.setCursor(0,1);
    lcd.print("Resolution: ");
    lcd.setCursor(16,3);
    lcd.print("INIT");
  }

  //print numOfPics
  lcd.setCursor(12,1);
  lcd.print(numOfPics);

  //clear dust
  for (int i=0; i < 3 - String(numOfPics).length(); i++)
    lcd.print(' ');
}


void takePictures() {
  double stepsByPic;
  double errorByMovement = modf((double)STEPS/numOfPics, &stepsByPic);
  double stepperAccumError = 0;
  int stepCounter = 0;

  //enable coilA & coilB
  digitalWrite(coilAPin, HIGH);
  digitalWrite(coilBPin, HIGH);

  for (int i=0; i < numOfPics ; i++) {
    //print info
    printProgressionInfo(stepCounter);

    //shoot camera
    digitalWrite(relePin, HIGH);
    delay(DELAY_CAMERA);
    digitalWrite(relePin, LOW);

    //move platform
    stepperAccumError += errorByMovement;
    int stepsByError = 0;
    if (stepperAccumError >= 1) {
      stepsByError = 1;
      stepperAccumError -= 1;
    }
    platform.step(stepsByPic + stepsByError);
    stepCounter += stepsByPic + stepsByError;

    //cancel operation
    if (checkButton())
      break;
  }

  //return to origin
  platform.step(STEPS - stepCounter);

  //disable coilA & coilB
  digitalWrite(coilAPin, LOW);
  digitalWrite(coilBPin, LOW);
}


void printProgressionInfo(int percent) {
  //compute
  int percentBar = map(percent, 0, STEPS, -1, 20);
  int percentNumber = map(percent, 0, STEPS, 0, 100);

  if (stateProgram != PROGRESSION) {
    stateProgram = PROGRESSION;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("GETTING PICS");
    lcd.setCursor(14,3);
    lcd.print("CANCEL");
  }

  //print progress information
  for (int i=0; i <= percentBar; i++)
    lcd.print(char(219));

  lcd.setCursor(9,2);
  lcd.print(percentNumber);
  lcd.print('%');
}


void loop() {
  encoderPtrVal(&numOfPics, 0, STEPS);
  mainMenu();

  if (checkButton()){
    enc.ptrVar = NULL;
    if (numOfPics > 0) {
      takePictures();
    }
  }
}
