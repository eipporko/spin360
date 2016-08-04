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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <Arduino.h>

typedef struct Variable {
  volatile short* ptrVar = NULL;
  int lowerVal;
  int higherVal;
} Variable_t;

typedef enum ButtonState {UNPRESSED, OK, CANCEL} ButtonState_t;

typedef struct Button {
  int val = LOW;
  int lastVal = LOW;
} Button_t;

typedef enum ProgramState {MENU, SETTINGS, MODIFY_SETTINGS, GETTING_PICS, ERROR_SETTINGS} ProgramState_t;

typedef struct Param {
  int address;
  volatile short val;
} Param_t;

#endif //__TYPES_H__

