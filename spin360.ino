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
#include <Encoder.h>
#include <Wire.h>
#include <math.h>

#define ERROR_MESSAGE_DELAY 1500
#define BUTTON_DEBOUNCE_DELAY 5
#define BUTTON_CANCEL_DELAY 500
#define STEPS 200

//Pins
const int ROTARY_A_PIN = 0;
const int ROTARY_B_PIN = 1;
const int ROTARY_SW_PIN = 4;
const int COIL_A_PIN = 5;
const int COIL_B_PIN = 10;
const int RELAY_PIN = 16;

volatile short numOfPics = 1;
volatile short setupIndex = 0;
Param_t param[2] = {
  {.address = 0, .val = 0},             //0: delay_camera
  {.address = sizeof(short), .val = 0}  //1: motor_speed
};

Variable_t var;
long oldPosition  = 0;

Encoder enc(ROTARY_A_PIN, ROTARY_B_PIN);
Button_t button;
ProgramState_t stateProgram = MENU;
bool newStateProgram = true;

Stepper platform(STEPS, 6, 7, 8, 9);
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  //init serial
  Serial.begin(9600);

  //init lcd
  lcd.init();
  lcd.backlight();
  lcd.noCursor();

  //setting pins
  pinMode(COIL_A_PIN, OUTPUT);
  pinMode(COIL_B_PIN, OUTPUT);
  pinMode(ROTARY_SW_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  //init outputs
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(COIL_A_PIN, LOW);
  digitalWrite(COIL_B_PIN, LOW);

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

void encoderPtrVal(volatile short* ptr, int low, int high) {
  var.lowerVal = low;
  var.higherVal = high;
  var.ptrVar = ptr;
}

void updatePtrVal() {
  long newPosition = enc.read();
  
  long i = (newPosition - oldPosition);
  if (i/4 != 0) { 
    *var.ptrVar += (i/4);
    if (*var.ptrVar > var.higherVal)
      *var.ptrVar = var.higherVal;
    else if (*var.ptrVar < var.lowerVal)
      *var.ptrVar = var.lowerVal;
    oldPosition = newPosition;
  }
  
}

bool buttonIsPressed() {
  button.lastVal = button.val;
  button.val = digitalRead(ROTARY_SW_PIN);

  if (button.val == LOW && button.lastVal == HIGH)
    return true;
  else
    return false;
}


ButtonState_t buttonCheckState() {
  if ( buttonIsPressed() ) {
    unsigned long startTime = millis();
    while (digitalRead(ROTARY_SW_PIN) == LOW ) {
      if (millis() - startTime > BUTTON_CANCEL_DELAY) {
        Serial.println("CANCEL");
        return CANCEL;
      }
    }

    unsigned long totalTime = millis() - startTime;

    if (totalTime > BUTTON_DEBOUNCE_DELAY) {
      Serial.println("OK");
      return OK;
    }
  }

  return UNPRESSED;
}


void modifySettingsMenu() {
  encoderPtrVal(&param[setupIndex].val, 0, 9999);
  updatePtrVal();
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
  updatePtrVal();
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
  int dustPosition = 1;
  if (setupIndex + 1 == 1)
    dustPosition = 2;
  else
    dustPosition = 1;
 
  lcd.setCursor(0, dustPosition);
  lcd.print(" ");
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
  updatePtrVal();

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
  double stepsByPic;
  double errorByMovement = modf((double)STEPS / numOfPics, &stepsByPic);
  double stepperAccumError = 0;
  int stepCounter = 0;

  //enable coilA & coilB
  digitalWrite(COIL_A_PIN, HIGH);
  digitalWrite(COIL_B_PIN, HIGH);

  for (int i = 0; i < numOfPics ; i++) {
    //print info
    printProgressionInfo(stepCounter);

    //shoot camera
    digitalWrite(RELAY_PIN, LOW);
    delay(param[0].val);
    digitalWrite(RELAY_PIN, HIGH);

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
  digitalWrite(COIL_A_PIN, LOW);
  digitalWrite(COIL_B_PIN, LOW);

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
  lcd.setCursor(0, 2);
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
