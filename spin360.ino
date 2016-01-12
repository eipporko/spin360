/*

   SPIN360

   Copyright (c) David Antunez Gonzalez 2016 <dantunezglez@gmail.com>

   All rights reserved.

   SPIN360 is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this library.

*/

#include "types.h"

#include <EEPROM.h>
#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <math.h>

#define ERROR_MESSAGE_DELAY 1500
#define BUTTON_DEBOUNCE_DELAY 5
#define BUTTON_CANCEL_DELAY 500
#define STEPS 200

//Pins
const int encAPin = 2;
const int encBPin = 3;
const int coilAPin = 4;
const int coilBPin = 9;
const int buttonPin = 10;
const int relePin = 11;

volatile short numOfPics = 1;
volatile short setupIndex = 0;
Param_t param[2] = {
  {.address = 0, .val = 0},             //0: delay_camera
  {.address = sizeof(short), .val = 0}  //1: motor_speed
};
Encoder_t enc;
Button_t button;
ProgramState_t stateProgram = MENU;
bool newStateProgram = true;

Stepper platform(STEPS, 5, 6, 7, 8);
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  //init serial
  Serial.begin(9600);

  //init lcd
  lcd.init();
  lcd.backlight();

  //setting pins
  pinMode(encAPin, INPUT);
  pinMode(encBPin, INPUT);
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

  //Get Params
  getParams();
}


int numOfParams() {
  return sizeof(param) / sizeof(Param_t);
}


void getParams() {

  for (int i = 0; i < numOfParams(); i++) {
    short variable = 0;
    EEPROM.get(param[i].address, variable);
    param[i].val = variable;
  }

  platform.setSpeed(param[1].val);
}


bool validateSettings() {
  if (param[0].val < 0 || param[1].val <= 0)
    return false;
  else
    return true;
}


void interruptEncAVal() {
  enc.sigAVal = digitalRead(encAPin);

  if (enc.ptrVar) {
    if (enc.sigAVal != enc.sigBVal)
      //clockwise
      *enc.ptrVar = (*enc.ptrVar < enc.higherVal) ? *enc.ptrVar++ : enc.higherVal;
    else
      //counterclockwise
      *enc.ptrVar = (*enc.ptrVar > enc.lowerVal) ? *enc.ptrVar-- : enc.lowerVal;
  }
}


void interruptEncBVal() {
  enc.sigBVal = digitalRead(encBPin);

  if (enc.ptrVar) {
    if (enc.sigBVal != enc.sigAVal)
      //counterclockwise
      *enc.ptrVar = (*enc.ptrVar > enc.lowerVal) ? *enc.ptrVar-- : enc.lowerVal;
    else
      //clockwise
      *enc.ptrVar = (*enc.ptrVar < enc.higherVal) ? *enc.ptrVar++ : enc.higherVal;
  }
}


void encoderPtrVal(volatile short* ptr, int low, int high) {
  enc.lowerVal = low;
  enc.higherVal = high;
  enc.ptrVar = ptr;
}


bool buttonIsPressed() {
  button.lastVal = button.val;
  button.val = digitalRead(buttonPin);

  if (button.val == HIGH && button.lastVal == LOW)
    return true;
  else
    return false;
}


ButtonState_t buttonCheckState() {
  if ( buttonIsPressed() ) {

    unsigned long startTime = millis();
    while (digitalRead(buttonPin) == HIGH ) {
      if (millis() - startTime > BUTTON_CANCEL_DELAY)
        return CANCEL;
    }

    unsigned long totalTime = millis() - startTime;

    if (totalTime > BUTTON_DEBOUNCE_DELAY)
      return OK;
  }

  return UNPRESSED;
}


void modifySettingsMenu() {
  encoderPtrVal(&param[setupIndex].val, 0, 9999);
  printModifySettingsMenu();

  ButtonState_t buttonState = buttonCheckState();
  switch (buttonState) {
    case OK:
      EEPROM.put(param[setupIndex].address, param[setupIndex].val);
      changeStateProgram(SETTINGS);
      break;
    case CANCEL:
      changeStateProgram(SETTINGS);
    default:
      break;
  }
}


void printModifySettingsMenu() {
  //print menu
  if (newStateProgram) {
    newStateProgram = false;
    lcd.setCursor(0, 3);
    lcd.print("CANCEL");
    lcd.setCursor(14, 3);
    lcd.print("  SAVE");
  }

  //print cursor
  lcd.setCursor(0, setupIndex + 1);
  lcd.print(' ');
  lcd.setCursor(15, setupIndex + 1);
  lcd.print(">");

  //print values
  lcd.setCursor(16, setupIndex + 1);
  lcd.print(param[setupIndex].val);

  //clear dust
  for (int i = 0; i < 4 - String(param[setupIndex].val).length(); i++)
    lcd.print(' ');
}


void settingsMenu() {
  encoderPtrVal(&setupIndex, 0, numOfParams() - 1);
  printSettingsMenu();

  ButtonState_t buttonState = buttonCheckState();
  switch (buttonState) {
    case OK:
      changeStateProgram(MODIFY_SETTINGS);
      break;
    case CANCEL:
      changeStateProgram(MENU);
    default:
      break;
  }
}


void printSettingsMenu() {

  //print menu
  if (newStateProgram) {
    newStateProgram = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SETTINGS MENU");
    lcd.setCursor(1, 1);
    lcd.print("Camera Delay:");
    lcd.setCursor(1, 2);
    lcd.print("Motor Speed:");
    lcd.setCursor(0, 3);
    lcd.print("RETURN");
    lcd.setCursor(14, 3);
    lcd.print("MODIFY");

    //Reload params from EEPROM
    getParams();
  }

  //print cursor
  lcd.setCursor(0, setupIndex + 1);
  lcd.print(">");

  //print values
  short variable = 0;
  lcd.setCursor(16, 1);
  lcd.print(param[0].val);
  lcd.setCursor(16 , 2);
  lcd.print(param[1].val);
}


void mainMenu() {
  encoderPtrVal(&numOfPics, 1, STEPS);
  printMainMenu();

  ButtonState_t buttonState = buttonCheckState();
  switch (buttonState) {
    case OK:
      if (validateSettings())
        changeStateProgram(GETTING_PICS);
      else
        changeStateProgram(ERROR_SETTINGS);
      break;
    case CANCEL:
      changeStateProgram(SETTINGS);
    default:
      break;
  }
}


void printMainMenu() {

  //print main menu
  if (newStateProgram) {
    newStateProgram = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MAIN MENU");
    lcd.setCursor(0, 1);
    lcd.print("Resolution:");
    lcd.setCursor(0, 3);
    lcd.print("SETTINGS");
    lcd.setCursor(16, 3);
    lcd.print("INIT");
  }

  //print numOfPics
  lcd.setCursor(12, 1);
  lcd.print(numOfPics);

  //clear dust
  for (int i = 0; i < 3 - String(numOfPics).length(); i++)
    lcd.print(' ');
}


void takePictures() {
  enc.ptrVar = NULL;

  double stepsByPic;
  double errorByMovement = modf((double)STEPS / numOfPics, &stepsByPic);
  double stepperAccumError = 0;
  int stepCounter = 0;

  //enable coilA & coilB
  digitalWrite(coilAPin, HIGH);
  digitalWrite(coilBPin, HIGH);

  for (int i = 0; i < numOfPics ; i++) {
    //print info
    printProgressionInfo(stepCounter);

    //shoot camera
    digitalWrite(relePin, HIGH);
    delay(param[0].val);
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
    if (buttonIsPressed())
      break;
  }

  //return to origin
  platform.step(STEPS - stepCounter);

  //disable coilA & coilB
  digitalWrite(coilAPin, LOW);
  digitalWrite(coilBPin, LOW);

  changeStateProgram(MENU);
}


void printProgressionInfo(int percent) {
  //compute
  int percentBar = map(percent, 0, STEPS, -1, 20);
  int percentNumber = map(percent, 0, STEPS, 0, 100);

  if (newStateProgram) {
    newStateProgram = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("GETTING PICS");
    lcd.setCursor(0, 3);
    lcd.print("CANCEL");
  }

  //print progress information
  lcd.setCursor(0, 1);
  for (int i = 0; i <= percentBar; i++)
    lcd.print(char(219));

  lcd.setCursor(9, 2);
  lcd.print(percentNumber);
  lcd.print('%');
}


void changeStateProgram(ProgramState_t newState) {
  stateProgram = newState;
  newStateProgram = true;
}

void errorSettingsMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ERROR!");
  lcd.setCursor(0, 1);
  lcd.print("   Check out your   ");
  lcd.setCursor(0,2);
  lcd.print("  Spin360 settings  ");
  
  delay(ERROR_MESSAGE_DELAY);
  changeStateProgram(MENU);
}

void loop() {

  switch (stateProgram) {
    case ERROR_SETTINGS:
      errorSettingsMessage();
      break;
    case MODIFY_SETTINGS:
      modifySettingsMenu();
      break;
    case SETTINGS:
      settingsMenu();
      break;
    case GETTING_PICS:
      takePictures();
      break;
    case MENU:
      mainMenu();
    default:
      break;
  }

}
