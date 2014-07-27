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
#include <AccelStepper.h>
#include <LiquidCrystal.h>
#include <eeprom.h>
#include "keypad.h"

/* Define PIN functions */
#define RELAY A3

/* Define menu states */
enum { MNU_SELECT_PRG, MNU_SELECT_RUN, MNU_SELECT_EDIT,
       MNU_RUN_COUNTDOWN, MNU_RUN_PRE_START, MNU_RUNNING, 
       MNU_SELECT_REWIND, MNU_SELECT_RETURN,
       MNU_REWIND, MNU_RETURN,
       MNU_EDIT_TYPE, MNU_EDIT_STEPS, MNU_EDIT_SPEED, MNU_EDIT_PRE_START,
       MNU_EDIT_LENGTH, MNU_EDIT_RADIUS, MNU_EDIT_CIRCUMFERENCE,
       MNU_EDIT_SAVE_NO, MNU_EDIT_SAVE_YES
};


/* EEPROM versioning */
const char version[] = "0001";

/* Programs types */
enum { PRG_EMPTY = 0, PRG_LINEAR, PRG_ROTARY, PRG_LAST };
char PRG_TYPES[4][9] = { "<Empty>", "<Linear>", "<Rotary>", ""};

/* These enum's are indexes into the Program_s structure */
enum { VAL_STEPS = 0, VAL_SPEED = 1, VAL_PRE_START = 2, VAL_LENGTH = 3,
	VAL_RADIUS = 3, VAL_CIRCUMFERENCE = 4 };
struct Program_s {
	uint8_t type;
	float	values[5];
};

union Program_u {
	Program_s P;
	char C[];
} Program;

/* How many programs can we store in FLASH */
int maxPrgs = (1024 - sizeof(version)) / sizeof(Program);

int state = MNU_SELECT_PRG;
int curPrg = 1;
int countdown;

KeyPad KEY(0);
LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7);
AccelStepper stepper(AccelStepper::DRIVER, A4, A5);

/* To allow printf to lcd */
static FILE lcdout = {0};
static int lcd_putchar(char ch, FILE* stream)
{
    lcd.write(ch) ;
    return (0) ;
}


/* map State to Program_s index */
uint8_t stateVal()
{
	switch( state )
	{
		case MNU_EDIT_STEPS: 
			return VAL_STEPS;
		case MNU_EDIT_SPEED: 
			return VAL_SPEED;
		case MNU_EDIT_PRE_START:
			return VAL_PRE_START;
       		case MNU_EDIT_LENGTH: 
			return VAL_LENGTH;
		case MNU_EDIT_RADIUS: 
			return VAL_RADIUS;
		case MNU_EDIT_CIRCUMFERENCE:
			return VAL_CIRCUMFERENCE;
		default:
			return 254;
	}
}

/* Start a Linear run */
void startLinear()
{
stepper.setMaxSpeed(100);
stepper.setAcceleration(20);
stepper.moveTo(50000);


}

/* Start a Rotary run */
void startRotary()
{


}

/* Erase EEPROM if version mismatch */
void initEEPROM()
{
	uint8_t t, diff=0;
	uint16_t i;
	// compare stored version to ours
	for( i = 0; i < sizeof(version); i++)
	{
		t = EEPROM.read(i);
		if( t != version[i] )
			diff++;
	}
			
	if( diff > 0 ) // mis match
	{
		lcd.println("Erasing EEPROM");
	
		// Blank program slots
		for ( i = sizeof(version); i < 1024; i++ )
		{
			t = EEPROM.read(i);
			if( t > 0) // only erase if not already 0
				EEPROM.write(i,0);
		}


		// Finally save our version 
		for( i = 0; i < sizeof(version); i++)
		{
			t = EEPROM.read(i);
			if( t != version[i] )
			  	EEPROM.write(i,version[i]);
		}
	}
}

/* load Program from curPrg slot in eeprom */
void loadProgram()
{
	uint16_t addr, i;
	addr = (curPrg * sizeof(Program)) - sizeof(Program) + sizeof(version);

	for( i = 0; i < sizeof(Program); i++)
	{
		Serial.print("Load ");
		Program.C[i]=EEPROM.read(addr + i);
		Serial.print(Program.C[i]);
		Serial.print(" from ");
		Serial.println(addr + i);
	}
}

/* save Program into curPrg slot in eeprom 
   Only write bytes that are different to save wear */
void saveProgram()
{
	uint16_t addr, i; 
	char t;
	addr = (curPrg * sizeof(Program)) - sizeof(Program) + sizeof(version);
	for( i = 0; i < sizeof(Program); i++)
	{
		Serial.print("Save  ");
		Serial.print(Program.C[i]);
		Serial.print(" at ");
		Serial.print(addr + i);
		t = EEPROM.read(addr + i);
		if (t != Program.C[i])
		{
			Serial.println("!");
			EEPROM.write(addr + i, Program.C[i]);
		}
		else
		{
			Serial.println(".");
		}
	}
}

void UpdateLCD()
{
	// Display top line
	lcd.setCursor(0,0);
	switch (state)
	{
		case MNU_SELECT_PRG:
		case MNU_SELECT_RUN:
		case MNU_SELECT_EDIT:
			fprintf(&lcdout,"%-16s","Program");
			break;
		case MNU_RUN_COUNTDOWN:
			fprintf(&lcdout,"Count Down %02d",(countdown / 1000) );
			break;
		case MNU_RUNNING:
			fprintf(&lcdout,"%-16s","Running");
			break;
		case MNU_RUN_PRE_START:
			fprintf(&lcdout,"Pre Start %04d", countdown );
			break;
		case MNU_SELECT_REWIND:
		case MNU_SELECT_RETURN:
			fprintf(&lcdout,"%-16s","Finished");
			break;
		case MNU_EDIT_TYPE:
			fprintf(&lcdout,"%-16s","Type");
			break;
		case MNU_EDIT_SAVE_YES:
		case MNU_EDIT_SAVE_NO:
			fprintf(&lcdout,"%-16s","Save Changes?");
			break;
		case MNU_EDIT_STEPS: 
			fprintf(&lcdout,"%-16s","Steps /mm");
			break;
		case MNU_EDIT_SPEED: 
			fprintf(&lcdout,"%-16s","Speed mm/s");
			break;
		case MNU_EDIT_PRE_START:
			fprintf(&lcdout,"%-16s","Start Delay s");
			break;
       		case MNU_EDIT_LENGTH: 
			fprintf(&lcdout,"%-16s","Distance mm");
			break;
		case MNU_EDIT_RADIUS: 
			fprintf(&lcdout,"%-16s","Radius mm");
			break;
		case MNU_EDIT_CIRCUMFERENCE:
			fprintf(&lcdout,"%-16s","Circumference mm");
			break;


			
	}

	// Display 2nd line
	lcd.setCursor(0,1);
	switch (state) 
	{
		case MNU_SELECT_PRG:
			fprintf(&lcdout," <%02d> Run Edit ",curPrg);
			break;
		case MNU_SELECT_RUN:
			fprintf(&lcdout,"  %02d <Run>Edit ",curPrg);
			break;
		case MNU_SELECT_EDIT:
			fprintf(&lcdout,"  %02d  Run<Edit>",curPrg);
			break;
		case MNU_RUN_PRE_START:
		case MNU_RUN_COUNTDOWN:
			fprintf(&lcdout,"%-16s"," ");
			break;
		case MNU_RUNNING:
			fprintf(&lcdout,"%-16s","<Abort>");
			break;
		case MNU_SELECT_REWIND:
			fprintf(&lcdout,"<Rewind> Return  ");
			break;
		case MNU_SELECT_RETURN:
			fprintf(&lcdout," Rewind <Return> ");
			break;
		case MNU_EDIT_TYPE:
			fprintf(&lcdout,"%-16s",PRG_TYPES[(int)Program.C[0]]);
			break;
		case MNU_EDIT_SAVE_YES:
			fprintf(&lcdout,"%-16s"," NO <YES>");
			break;
		case MNU_EDIT_SAVE_NO:
			fprintf(&lcdout,"%-16s","<NO> YES");
			break;
		case MNU_EDIT_STEPS: 
		case MNU_EDIT_SPEED: 
		case MNU_EDIT_PRE_START:
       		case MNU_EDIT_LENGTH: 
		case MNU_EDIT_RADIUS: 
		case MNU_EDIT_CIRCUMFERENCE:
			fprintf(&lcdout,"< %07.2f >     ",
					Program.P.values[stateVal()]);
			break;


	}
}


void setup()
{
  digitalWrite(RELAY,LOW);
  pinMode(RELAY,OUTPUT);
  digitalWrite(A4,LOW);
  pinMode(A4,OUTPUT);
  digitalWrite(A5,LOW);
  pinMode(A5,OUTPUT);
  lcd.begin(16, 2);
/* To allow printf to lcd */
  fdev_setup_stream (&lcdout, lcd_putchar, NULL, _FDEV_SETUP_WRITE);
  lcd.clear();
  Serial.begin(9600);
  initEEPROM();
  UpdateLCD();
}

void loop() {
	static int ms = 0;
	static uint8_t oldkey = BTN_NONE;
	static unsigned long tm_last = 0;
	uint8_t key = BTN_NONE;
	unsigned long tm_now = millis();

	stepper.run();
Serial.print(tm_now);
Serial.print(" ");
Serial.print(tm_last);
Serial.print(" ");
Serial.println(ms);
	ms = (tm_now - tm_last);
	countdown -= ms;
/*
	ms += (tm_now - tm_last);
	if( ms > 999 )
	{
		if( countdown > 0)
			countdown--;	

		ms -= 1000;
	}
*/
	key = KEY.read();
	switch( state )
	{
		case MNU_SELECT_PRG:
			switch( key )
			{
				case BTN_UP:
					curPrg += KEY.HoldMultiplier(10);
					if( curPrg > maxPrgs) 
						curPrg = 1;
					loadProgram();
					break;
				case BTN_DOWN:
					curPrg -= KEY.HoldMultiplier(10);
					if( curPrg < 1) 
						curPrg = maxPrgs;
					loadProgram();
					break;
				case BTN_RIGHT:
					if( Program.P.type == PRG_EMPTY)
						state = MNU_SELECT_EDIT;
					else
						state = MNU_SELECT_RUN;
					break;
			}
			break;
		case MNU_SELECT_RUN:
			switch( key )
			{
				case BTN_LEFT:
					state = MNU_SELECT_PRG;
					break;
				case BTN_RIGHT:
					state = MNU_SELECT_EDIT;
					break;
				case BTN_SELECT:
					state = MNU_RUN_COUNTDOWN;
					countdown = 5500; // 5 seconds
					break;
			}
			break;
		case MNU_SELECT_EDIT:
			switch( key )
			{
				case BTN_LEFT:
					if( Program.P.type == PRG_EMPTY)
						state = MNU_SELECT_PRG;
					else
						state = MNU_SELECT_RUN;
					break;
				case BTN_SELECT:
					state = MNU_EDIT_TYPE;
					break;
			}
			break;
		case MNU_RUN_COUNTDOWN:
			if( countdown < 1 ) 
			{
				// Turn relay on 
				digitalWrite(RELAY,HIGH);
				state = MNU_RUN_PRE_START;
				countdown = Program.P.values[VAL_PRE_START];
			}
			break;
		case MNU_RUN_PRE_START:
			if( countdown < 1) 
			{
				if(Program.P.type == PRG_LINEAR) 
				{
					startLinear();
					state = MNU_RUNNING;
				} 
				else if (Program.P.type == PRG_ROTARY)
				{
					startRotary();
					state = MNU_RUNNING;
				}
					
digitalWrite(A3,LOW);
			// TODO start stepper running
			}
			break;
		case MNU_RUNNING:
			if (key != BTN_NONE)
			{
				state = MNU_SELECT_REWIND;
			} 
			// TODO check if still running
			break;
		case MNU_SELECT_REWIND:
			switch( key )
			{
				case BTN_RIGHT:
					state = MNU_SELECT_RETURN;
					break;
				case BTN_SELECT:
					// TODO start stepper rewinding
					state = MNU_REWIND;
					break;
			}
			break;
		case MNU_SELECT_RETURN:
			switch( key )
			{
				case BTN_LEFT:
					state = MNU_SELECT_REWIND;
					break;
				case BTN_SELECT:
					state = MNU_RETURN;
					break;
			}
			break;
		case MNU_REWIND:
			// TODO rewind (like running)
			state = MNU_SELECT_RUN;
			break;
		case MNU_RETURN:
			state = MNU_SELECT_RUN;
			break;
		case MNU_EDIT_TYPE:
			switch( key )
			{
				case BTN_UP:
					Program.P.type++;
					if (Program.P.type == PRG_LAST)
						Program.P.type=PRG_EMPTY;
					break; 
				case BTN_DOWN:  
					if (Program.P.type == PRG_EMPTY)
						Program.P.type = PRG_LAST;
					
					Program.P.type--;
					break; 
				case BTN_RIGHT:
				case BTN_SELECT:
					if( Program.P.type != PRG_EMPTY )
						state++;
					break;
				case BTN_LEFT:
					state = MNU_EDIT_SAVE_NO;
					break;
					
			}
			break;

		case MNU_EDIT_STEPS: 
		case MNU_EDIT_SPEED: 
		case MNU_EDIT_PRE_START:
       		case MNU_EDIT_LENGTH: 
		case MNU_EDIT_RADIUS: 
		case MNU_EDIT_CIRCUMFERENCE:
			switch( key )
			{
				case BTN_RIGHT:
					if (Program.P.type == PRG_ROTARY && 
						    state == MNU_EDIT_PRE_START)
						state = MNU_EDIT_RADIUS;
					else if (Program.P.type == PRG_LINEAR &&
						       state == MNU_EDIT_LENGTH)
						state = MNU_EDIT_SAVE_NO;
					else
						state++;
					break;
				case BTN_LEFT:
					if (Program.P.type == PRG_ROTARY &&
						      state == MNU_EDIT_RADIUS)
						state = MNU_EDIT_PRE_START;
					else
						state--;
					break;
				case BTN_UP:
					Program.P.values[stateVal()] += 
						0.01 * KEY.HoldMultiplier();
					break;
				case BTN_DOWN:
					Program.P.values[stateVal()] -= 
						0.01 * KEY.HoldMultiplier();
					break;
			}
			if( Program.P.values[stateVal()] < 0.0 )
				Program.P.values[stateVal()] = 9999.99;
			else if( Program.P.values[stateVal()] >= 10000)
				Program.P.values[stateVal()] = 0;
			break;
		case MNU_EDIT_SAVE_NO:
			switch( key )
			{
				case BTN_RIGHT:
					state = MNU_EDIT_SAVE_YES;
					break;
				case BTN_SELECT:
					loadProgram();
					state = MNU_SELECT_PRG;
					break;
			}
			break;
		case MNU_EDIT_SAVE_YES:
			switch( key )
			{
				case BTN_LEFT:
					state = MNU_EDIT_SAVE_NO;
					break;
				case BTN_SELECT:
					saveProgram();
					state = MNU_SELECT_PRG;
					break;
			}
			break;
		break;
			

	}
	UpdateLCD();
	tm_last = tm_now;
}




