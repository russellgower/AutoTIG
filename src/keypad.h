/*
* Copyright (C) Russell Gower 2014
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef KEYPAD_H
#define KEYPAD_H
#include "Arduino.h"
#include <inttypes.h>
enum { BTN_SELECT, BTN_LEFT, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_NONE};

const int adc_key_val[5][2] = {
				{800, BTN_SELECT},
				{600, BTN_LEFT},
				{400, BTN_DOWN},
				{200, BTN_UP},
				{50,  BTN_RIGHT}
			  };

class KeyPad {
public:
	KeyPad(uint8_t pin);
	uint8_t read();
	int HoldMultiplier(int max = 0);
	
private:
	int m_holdMultiplier;
	int m_threshold;
	unsigned int m_debounce;
	uint8_t m_pin;

};
	

#endif
