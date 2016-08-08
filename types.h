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
#include <EEPROM.h>

typedef enum ButtonState {UNPRESSED, OK, CANCEL} ButtonState_t;

typedef struct Button {
  int val = LOW;
  int lastVal = LOW;
} Button_t;

typedef enum ProgramState {MENU, SETTINGS, MODIFY_SETTINGS, GETTING_PICS, ERROR_SETTINGS} ProgramState_t;

typedef struct Param {
  int address;
  String descriptor;
  volatile short data;
  int minVal;
  int maxVal;

  Param(String descriptor, int address, int data, int minVal, int maxVal):
    descriptor(descriptor),
    address(address),
    data(data),
    minVal(minVal),
    maxVal(maxVal)
  {};

  Param(String descriptor, int data, int minVal, int maxVal):
    Param(descriptor, -1, data, minVal, maxVal)
  {};
  
  Param(int data, int minVal, int maxVal):
    Param("", -1, data, minVal, maxVal)
  {};

  setMinVal(int newMinVal) {
    minVal = newMinVal;
  }

  setMaxVal(int newMaxVal) {
    maxVal = newMaxVal;
  }
    
  Param& operator+(const int i) {
    if (i > 0)
      (data + i <= maxVal ) ? data += i : data = maxVal;
    else
      (data + i >= minVal ) ? data += i : data = minVal;
      
    return *this;
  }

  Param& operator+=(const int i) {
    Param result = *this + i;
    return result;
  }
  
  Param& operator++() {
    (data < maxVal) ? data++ : data = maxVal;
    return *this;
  }

  Param operator++(int) {
    Param result(*this);
    ++(*this);
    return result;
  }

  Param& operator--(){
    (data > minVal) ? data-- : data = minVal;
    return *this;
  }

  Param operator--(int) {
    Param result(*this);
    --(*this);
    return result;
  }

  void save() {
    if (address != -1)
      EEPROM.put(address, data);
  }

  template< typename T > T &get(T &t) {
    if (address != -1)
      EEPROM.get(address, t);
    return t;
  }
  
} Param_t;

#endif //__TYPES_H__

