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

#include <Stepper.h>
#include <LiquidCrystal_I2C.h>
#include <Encoder.h>
#include <Wire.h>
#include <math.h>

#define ERROR_MESSAGE_DELAY 1500
#define BUTTON_DEBOUNCE_DELAY 5
#define BUTTON_CANCEL_DELAY 500

int numOfParams();

//Pins
const int ROTARY_A_PIN = 0;
const int ROTARY_B_PIN = 1;
const int ROTARY_SW_PIN = 4;
const int COIL_A_PIN = 5;
const int COIL_B_PIN = 10;
const int RELAY_PIN = 16;

Param_t param[3] = {
  Param_t{"Shot Delay: ", 0, 0, 0, 9999},
  Param_t{"Motor Speed:", sizeof(short), 0, 0, 9999},
  Param_t{"Motor Steps:", sizeof(short)*2, 0, 0, 9999}
};

Param_t numOfPics("Resolution:", 0, 0, 0);
Param_t setupIndex(0, 0, numOfParams()-1);
int setupWindow[2] = {0, 1};


long oldPosition  = 0;

Encoder enc(ROTARY_A_PIN, ROTARY_B_PIN);
Button_t button;
Param_t *ptrParam;

ProgramState_t stateProgram = MENU;
bool newStateProgram = true;

Stepper *platform = NULL;
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
    param[i].get(variable);
    param[i].data = variable;
  }

  platform = new Stepper(param[2].data, 6, 7, 8, 9);
  platform->setSpeed(param[1].data);

  if (param[2].data > 0) {
    numOfPics.data = 1;
    numOfPics.setMinVal(1);
    numOfPics.setMaxVal(param[2].data);
  }
}


bool validateSettings() {
  if (param[0].data < 0 || param[1].data <= 0 || param[2].data <= 0)
    return false;
  else
    return true;
}

void updatePtrVal() {

  if (ptrParam != NULL) {
    long newPosition = enc.read();
  
    long i = (newPosition - oldPosition);
    if (i/4 != 0) { 
      *ptrParam += (i/4);
      oldPosition = newPosition;
    }
  }
}

void updateSetupWindow() {
  int index = setupIndex.data;
  int lastIndexParam;
  int windowLenght = sizeof(setupWindow)/sizeof(int);
  for (int i = 0; i < windowLenght; i++) {
    lastIndexParam = setupWindow[i];
    if (setupWindow[i] == setupIndex.data)
      return;
  }
  
  int offset = (abs(index - setupWindow[0]) < abs(index - lastIndexParam)) ? 
    index - setupWindow[0] : index - lastIndexParam;

  for (int i = 0; i < windowLenght; i++) {
    setupWindow[i] += offset; 
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
        return CANCEL;
      }
    }

    unsigned long totalTime = millis() - startTime;

    if (totalTime > BUTTON_DEBOUNCE_DELAY) {
      return OK;
    }
  }

  return UNPRESSED;
}


void modifySettingsMenu() {
  ptrParam = &param[setupIndex.data];
  updatePtrVal();
  printModifySettingsMenu();

  ButtonState_t buttonState = buttonCheckState();
  switch (buttonState) {
    case OK:
      param[setupIndex.data].save();
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

  int cursorPos = 0;
  int windowLenght = sizeof(setupWindow)/sizeof(int);
  for (int i = 0; i < windowLenght; i++) {
    if (setupWindow[i] == setupIndex.data) {
      cursorPos = i;
    }
  }
  
  //print cursor
  lcd.setCursor(0, cursorPos + 1);
  lcd.print(' ');
  lcd.setCursor(15, cursorPos + 1);
  lcd.print(">");

  //print values
  lcd.setCursor(16, cursorPos + 1);
  lcd.print(param[setupWindow[cursorPos]].data);

  //clear dust
  for (int i = 0; i < 4 - String(param[setupIndex.data].data).length(); i++)
    lcd.print(' ');
}


void settingsMenu() {
  ptrParam = &setupIndex;
  updatePtrVal();
  updateSetupWindow();
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
    lcd.setCursor(0, 3);
    lcd.print("RETURN");
    lcd.setCursor(14, 3);
    lcd.print("MODIFY");

    //Reload params from EEPROM
    getParams();
  }

  int cursorPos = 0;
  int windowLenght = sizeof(setupWindow)/sizeof(int);
  for (int i = 0; i < windowLenght; i++) {
    lcd.setCursor(1, i+1);
    lcd.print(param[setupWindow[i]].descriptor);

    lcd.setCursor(16, i+1);
    lcd.print(param[setupWindow[i]].data);

    //clear dust
    int dustPositions = 4 - String(param[setupWindow[i]].data).length();
    for (int i = 0; i < dustPositions; i++) {
      lcd.print(' ');
    }

    if (setupWindow[i] == setupIndex.data) {
      cursorPos = i;
    }
  }

  //print cursor
  int dustPosition = 1;
  if (cursorPos == 0)
    dustPosition = 2;
  else
    dustPosition = 1;
 
  lcd.setCursor(0, dustPosition);
  lcd.print(" ");
  lcd.setCursor(0, cursorPos + 1);
  lcd.print(">");
}


void mainMenu() {
  ptrParam = &numOfPics;
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
    lcd.print(numOfPics.descriptor);
    lcd.setCursor(0, 3);
    lcd.print("SETTINGS");
    lcd.setCursor(16, 3);
    lcd.print("INIT");
  }

  //print numOfPics
  lcd.setCursor(12, 1);
  lcd.print(numOfPics.data);

  //clear dust
  for (int i = 0; i < 3 - String(numOfPics.data).length(); i++)
    lcd.print(' ');
}


void takePictures() {
  ptrParam = NULL;
  double stepsByPic;
  double errorByMovement = modf((double)param[2].data / numOfPics.data, &stepsByPic);
  double stepperAccumError = 0;
  int stepCounter = 0;

  //enable coilA & coilB
  digitalWrite(COIL_A_PIN, HIGH);
  digitalWrite(COIL_B_PIN, HIGH);

  for (int i = 0; i < numOfPics.data ; i++) {
    //print info
    printProgressionInfo(stepCounter);

    //shoot camera
    digitalWrite(RELAY_PIN, LOW);
    delay(param[0].data);
    digitalWrite(RELAY_PIN, HIGH);

    //move platform
    stepperAccumError += errorByMovement;
    int stepsByError = 0;
    if (stepperAccumError >= 1) {
      stepsByError = 1;
      stepperAccumError -= 1;
    }
    if (platform != NULL)
      platform->step(stepsByPic + stepsByError);
    stepCounter += stepsByPic + stepsByError;

    //cancel operation
    if (buttonIsPressed())
      break;
  }

  //return to origin
  if (platform != NULL)
    platform->step(param[2].data - stepCounter);

  //disable coilA & coilB
  digitalWrite(COIL_A_PIN, LOW);
  digitalWrite(COIL_B_PIN, LOW);

  changeStateProgram(MENU);
}


void printProgressionInfo(int percent) {
  //compute
  int percentBar = map(percent, 0, param[2].data, -1, 20);
  int percentNumber = map(percent, 0, param[2].data, 0, 100);

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
