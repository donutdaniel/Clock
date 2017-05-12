#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "lcd.h"

/*GLOBAL VARIABLES */

//Time
volatile unsigned char hourTens;
volatile unsigned char hourOnes;
volatile unsigned char minuteTens;
volatile unsigned char minuteOnes;
volatile unsigned char secondTens;
volatile unsigned char secondOnes;
//day, 1 is Monday
volatile unsigned char day;
char* days[7];

//Alarm
volatile unsigned char alarmHourTens;
volatile unsigned char alarmHourOnes;
volatile unsigned char alarmMinuteTens;
volatile unsigned char alarmMinuteOnes;

//state of clock
volatile unsigned char displayState; //0: Normal, 1: Minute, 2: Hour, 3: Day, 4:Alarm Hour, 5:Alarm Minute
volatile char displayChanged; //false

//encoder variables
volatile unsigned char a, b;
volatile unsigned char new_state, old_state;
volatile unsigned char encoderChanged;

//buzzer timer variables
volatile int counter;

//serial variables
volatile unsigned char dataBuffer[5];
volatile unsigned char startFlag; //0 for off, 1 for start
volatile unsigned char dataBufferSize; //0 to 4
volatile unsigned char validFlag; //0 for false, 1 for true

/*FUNCTIONS*/
void init_timer();
void init_clock();
void init_buzzer();

void setHourDisplay();
void setMinuteDisplay();
void setDayDisplay();
void setAlarmHour();
void setAlarmMinute();
void displayNormal();
// Takes in two number chars 0-9, and outputs them to the screen
void displayTemperatureIn(unsigned char a, unsigned char b);
void displayTemperatureOut(unsigned char a, unsigned char b);
// Converts from Celcius to Fahrenheit
void tempConvert(unsigned char* t);
void tx_char(char ch);