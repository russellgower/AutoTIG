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

#include "keypad.h"

KeyPad::KeyPad(uint8_t pin) {
	m_pin            = pin;
	m_holdMultiplier = 1;
	m_threshold      = 60;
	m_debounce 	 = 120;
	
	//pinMode(pin, INPUT);
	//digitalWrite(pin, HIGH);
}

uint8_t KeyPad::read() {

	static unsigned long lastTime = millis();
	static unsigned int  held = 0;
	static uint8_t       oldBTN = BTN_NONE;
	static uint8_t       holdBTN = BTN_NONE;
	static int           oldADC = 0;

	uint8_t newBTN = BTN_NONE;
	int newADC = analogRead(m_pin);
/*
	// filter out any analog jitter
	if( abs(oldADC - newADC) > m_threshold ) {
		oldADC = newADC;
		return BTN_NONE;
	}
*/
	// map adc value to button code
	for( uint8_t i = 0; i <5; i++ ) {
		if ( newADC < adc_key_val[i][0] ) 
			newBTN = adc_key_val[i][1];
	}
	
	if( newBTN != oldBTN ) { // set up for debouncing
		oldBTN=newBTN;
		holdBTN=BTN_NONE;
		lastTime = millis();
		held = 0;
		m_holdMultiplier = 1;
		return BTN_NONE;
	}

	// debounce?
	if( millis() - lastTime > m_debounce) 
		lastTime = millis();
	else
		return BTN_NONE; // not long enough yet


	// do we need to increase the multiplier?
	if ( newBTN == holdBTN && newBTN != BTN_NONE) {
		held++;

		if (held > 9) {
			if( m_holdMultiplier < 10000)
				m_holdMultiplier *= 10;
			held = 0;
		}

	} else {
		holdBTN = newBTN;
	}

	return newBTN;
}
			


int KeyPad::HoldMultiplier(int max ) {
	if( max > 0 and m_holdMultiplier > max)
		return max;
	else
		return m_holdMultiplier;
}
