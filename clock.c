/********************************************
*
*  Name: Hsiao Tung Ho
*  Section: 2pm
*  Assignment: Clock project
*
********************************************/

#ifndef CLOCK_C
#define CLOCK_C

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "lcd.h"
#include "clock.h"
#include "encoder.h"
#include "serial.h"
#include "ds1631.h"

//main program
int main(){
	init_lcd();
	init_timer(); //timer for clock and buzzer
 	init_clock();
 	init_buzzer();
 	init_encoder(); 
 	init_buttons(); 
 	init_serial();
	ds1631_init();	//enable temp sensor
	ds1631_conv(); //continuous 

	//init led for hotter/colder
	DDRC |= (1 << PC1 | 1 << PC2); // Set pc to output
	PORTC &= ~(1 << PC1 | 1 << PC2); // Output off for both;
	//variables to compare temp
	int tempInInt = 0;
	int tempOutInt = 0;

	unsigned char tempIn[2]; //array for temp
	int tempState = 0; //so only changes if temp is changed, no lcd flashing

	displayNormal(); //initial display

	//initial remote temp:
	displayTemperatureOut(0x00, 0x00);
	
	while(1){
		// Check if change screen
		if(displayChanged || encoderChanged){
		  	if(displayState == 1){
		  		setHourDisplay();
		  	}else if(displayState == 2){
		  		setMinuteDisplay();
		  	}else if(displayState == 3){
		  		setDayDisplay();
		  	}else if(displayState == 4){
		  		setAlarmHour();
		  	}else if(displayState == 5){
		  		setAlarmMinute();
		  	}else if(displayState == 0){ //state 0
		  		displayNormal();
		  		tempConvert(tempIn); //conv to fahrenheit
				displayTemperatureIn(tempIn[0], tempIn[1]);
				displayTemperatureOut(dataBuffer[1], dataBuffer[2]);
		  	}
		  	displayChanged = 0;
		  	encoderChanged = 0;
		}

		// Temp conversion and display (faster than above)
		ds1631_temp(tempIn);
		if(tempState != tempIn[0]+tempIn[1] && displayState == 0){ //temperature changed
			tempState = tempIn[0]+tempIn[1];
			tempConvert(tempIn); //conv to fahrenheit
			displayTemperatureIn(tempIn[0], tempIn[1]);

			// Change to int for comparison
			tempInInt = (int)(tempIn[0]*10) + (int)(tempIn[1]);
			//sscanf(tempInFah, "%d", &tempInInt);

			// Transmit fahrenheit data to remote unit
			tx_char('#');
			if(tempInInt >= 0){
				tx_char('+');
			}else{
				tx_char('-');
			}
			tx_char(tempIn[0] + '0');
			tx_char(tempIn[1] + '0');
			tx_char('$');
		}

		//check for data recieve
		if(validFlag == 1 && displayState == 0){
			//display
			validFlag = 0;

			displayTemperatureOut(dataBuffer[1], dataBuffer[2]);

			// Change to int for comparison
			tempOutInt = (int)(dataBuffer[1]*10) + (int)(dataBuffer[2]);
			//sscanf(dataBuffer, "%i", &tempOutInt);
		}

		// Check hotter/colder (led)
		if(tempInInt > tempOutInt){ // Hotter inside
			PORTC |= (1 << PC2);
			PORTC &= ~(1 << PC1);
		}else if(tempInInt < tempOutInt){ // Hotter outside
			PORTC |= (1 << PC1);
			PORTC &= ~(1 << PC2);
		}else{ // Equal temperature
			PORTC |= (1 << PC1 | 1 << PC2);
		}
	}
	return 0;
}

//interface for HOUR select
void setHourDisplay(){
	writecommand(1); //clear screen
	moveto(0,0);
	stringout("Hour:");
	moveto(0,8);
	writedata(hourTens + '0');
	writedata(hourOnes + '0');
}
//interface for MINUTE select
void setMinuteDisplay(){
	writecommand(1); //clear screen
	moveto(0,0);
	stringout("Minute:");
	moveto(0,8);
	writedata(minuteTens + '0');
	writedata(minuteOnes + '0');
}
//interface for DAY select
void setDayDisplay(){
	writecommand(1); //clear screen
	moveto(0,0);
	stringout("Day:");
	moveto(0,8);
	stringout(days[day]);
}

void setAlarmHour(){
	writecommand(1); //clear screen
	moveto(0,0);
	stringout("Alarm Hour:");
	moveto(0,14);
	writedata(alarmHourTens + '0');
	writedata(alarmHourOnes + '0');
}

void setAlarmMinute(){
	writecommand(1); //clear screen
	moveto(0,0);
	stringout("Alarm Minute:");
	moveto(0,14);
	writedata(alarmMinuteTens + '0');
	writedata(alarmMinuteOnes + '0');
}

void displayNormal(){
	writecommand(1);
	//write day
	moveto(0,0);
	stringout(days[day]);

	//write time
	moveto(0,8);
	writedata(hourTens + '0');
	writedata(hourOnes + '0');
	writedata(':');
	writedata(minuteTens + '0');
	writedata(minuteOnes + '0');
	writedata(':');
	writedata(secondTens + '0');
	writedata(secondOnes + '0');
}

void displayTemperatureIn(unsigned char a, unsigned char b){
	moveto(1,0);
	stringout("In: ");
	moveto(1,4);
	writedata(a + '0');
	moveto(1,5);
	writedata(b + '0');
}

void displayTemperatureOut(unsigned char a, unsigned char b){
	moveto(1,9);
	stringout("Out: ");
	writedata(a + '0');
	writedata(b + '0');
}

void tempConvert(unsigned char* t){
	//converts t[0] and t[1] to fahrenheit
	//also saves it in t2
	int temp = t[0] * 10;
	if(t[1] != 0){ //half
		temp += 5;
	} //temp is now mult. by 10
	temp *= 9;
	temp /= 50;
	temp += 32;
	t[1] = temp % 10;
	t[0] = temp / 10;
}

void tx_char(char ch){
	//Wait for transmitter data register empty
	while((UCSR0A & (1 << UDRE0)) == 0){ 
		//do nothing
	}
	UDR0 = ch;
}

//timer of both clock and buzzer
void init_timer(){
  /*clock timer, Timer1*/
  //Set to CTC mode
  TCCR1B |= (1 << WGM12);
  //Enable timer interrupt
  TIMSK1 |= (1 << OCIE1A);
  //load max count prescalar 256, 1Hz
  OCR1A = 62500;
  //prescalar = 256
  TCCR1B |= (1 << CS12);

  /*buzzer timer, Timer0*/
  //Set to CTC mode
  TCCR0A |= (1 >> WGM01);
  //Enable timer interrupt
  TIMSK0 |= (1 << OCIE0A);
  //Compare match register. Max 255, 1000Hz
  OCR0A = 250;
  //prescalar = 0. Optimal would be 64, which is CS02=0, CS01=1, CS00=1
  TCCR0B &= ~(1 << CS02 | 1 << CS01 | 1 << CS00);

  //enable interrupts
  sei();
}

void init_clock(){
  init_lcd();
  writecommand(1);

  //load time
  hourTens = eeprom_read_byte((void *) 0);
  hourOnes = eeprom_read_byte((void *) 1);
  minuteTens = eeprom_read_byte((void *) 2);
  minuteOnes = eeprom_read_byte((void *) 3);
  secondTens = eeprom_read_byte((void *) 4);
  secondOnes = eeprom_read_byte((void *) 5);
  //from 0-6
  day = eeprom_read_byte((void *) 6);
  days[0] = "Sun.";
  days[1] = "Mon.";
  days[2] = "Tues.";
  days[3] = "Weds.";
  days[4] = "Thurs.";
  days[5] = "Fri.";
  days[6] = "Sat.";

  /*check for valid eeprom (time). 
    If not valid, then use default(00:00:00)*/
  if(hourTens == 0xFF || hourOnes == 0xFF || minuteTens == 0xFF || minuteOnes == 0xFF 
  	|| secondTens == 0xFF || secondOnes == 0xFF || day == 0xFF){
  	//incorrect load. use default
  	hourTens = 1;
  	hourOnes = 2;
  	minuteTens = 0;
  	minuteOnes = 0;
  	secondTens = 0;
  	secondOnes = 0;
  	day = 0;
  }

  //load Alarm
  alarmHourTens = eeprom_read_byte((void *) 7);
  alarmHourOnes = eeprom_read_byte((void *) 8);
  alarmMinuteTens = eeprom_read_byte((void *) 9);
  alarmMinuteOnes = eeprom_read_byte((void *) 10);

  /*check for valid eeprom (alarm). 
    If not valid, then use default(00:00:00)*/
  if(alarmHourTens == 0xFF || alarmHourTens == 0xFF || 
  	alarmMinuteTens == 0xFF || alarmMinuteOnes == 0xFF){
  	//incorrect load. use default
	alarmHourTens = 1;
	alarmHourOnes = 2;
	alarmMinuteTens = 0;
	alarmMinuteOnes = 0;
  }

  displayChanged = 1; //so can update screen
}

/*SET IN PB5*/
void init_buzzer(){
	DDRB |= (1 << PB5); // Set PC1 to output
	PORTB &= ~(1 << PB5); // Output 0
	counter = 0;
}

//Time increment, triggers every second
ISR(TIMER1_COMPA_vect){
	secondOnes += 1;
	if(secondOnes >= 10){
	  secondOnes = 0;
	  secondTens += 1;
	}
	if(secondTens >= 6){
	  secondTens = 0;
	  minuteOnes += 1;
	  //save to eeprom
	  eeprom_update_byte((void *) 0, hourTens);
	  eeprom_update_byte((void *) 1, hourOnes);
	  eeprom_update_byte((void *) 2, minuteTens);
	  eeprom_update_byte((void *) 3, minuteOnes);
	  eeprom_update_byte((void *) 4, secondTens);
	  eeprom_update_byte((void *) 5, secondOnes);
	  eeprom_update_byte((void *) 6, day);
	}
	if(minuteOnes >= 10){
	  minuteOnes = 0;
	  minuteTens += 1;
	}
	if(minuteTens >= 6){
	  minuteTens = 0;
	  hourOnes += 1;
	}
	if(hourOnes >= 10){
	  hourOnes = 0;
	  hourTens += 1;
	}
	if(hourTens >= 2 && hourOnes >= 5){
		hourTens = 0;
		hourOnes = 1;
		day += 1;
		if(day == 7){
			day = 0;
		}
	}
  //write time
	if(displayState == 0){
		moveto(0,0);
		stringout(days[day]);
		moveto(0,8);
		writedata(hourTens + '0');
		writedata(hourOnes + '0');
		writedata(':');
		writedata(minuteTens + '0');
		writedata(minuteOnes + '0');
		writedata(':');
		writedata(secondTens + '0');
		writedata(secondOnes + '0');
	}

	//alarm check
	if(hourTens == alarmHourTens && hourOnes == alarmHourOnes && 
		minuteTens == alarmMinuteTens && minuteOnes == alarmMinuteOnes &&
		secondTens == 0 && secondOnes == 0){
		//output buzzer by turning on counter
		//set prescalar to 66, which is 011
		TCCR0B |= (1 << CS01 | 1 << CS00);
	}
	
}

//Buzzer increment, triggers 1000 times per second
ISR(TIMER0_COMPA_vect){
	//turn on buzzer
	if(counter>=0){ //to prevent buzzing during snooze
		PORTB ^= (1 << PB5); //turn on and off to get frequency
	}else{
		PORTB &= ~(1 << PB5);
	}
	counter++;
	//reached 5 seconds, turn off and end.
	if(counter == 5000){
		counter = 0;
		//turn off buzzer
		PORTB &= ~(1 << PB5);
		//set prescalar to all 0's, turned off timer.
  		TCCR0B &= ~(1 << CS02 | 1 << CS01 | 1 << CS00);
	}
}

#endif