#ifndef ENCODER_C
#define ENCODER_C

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "clock.h"
#include "encoder.h"

/*BASED IN PD2(timeset) AND PD3(snooze)*/
void init_buttons(){
	PCICR |= (1 << PCIE2); //enable pin change interrupts on portd
	PCMSK2 |= (1 << PCINT18 | 1 << PCINT19); //enable interrupts for PD2, PD3 
	sei(); //enable global interrupts

	/*begin init. of button*/
	DDRD &= ~(1 << PD2 | 1 << PD3); //set to input
	PORTD |= (1 << PD2 | 1 << PD3); //enable pullup resistor in PD2, PD3
	//if PIND is 1, then not pressed. 0 is pressed.

	displayState = 0;
	displayChanged = 0;
}

void init_encoder(){
	//BASED IN PB3, PB4 (D11, D12 respectively)
	PCICR |= (1 << PCIE0); //enable pin change interrupts on portb
	PCMSK0 |= (1 << PCINT3 | 1 << PCINT4); //enable interrupts for PB3, PB4
	sei(); //enable global interrupts
	/*init of inputs*/
	DDRB &= ~(1 << PB3 | 1 << PB4); //set PB3, PB4 to input
	PORTB |= (1 << PB3 | 1 << PB4); //enable pullup resistor in PC4, PC5

    // Determine the intial state
    a = PINB & (1 << PB3);
    b = PINB & (1 << PB4);

    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;
    encoderChanged = 0;
}

/*Change on PORTD (Buttons)*/
ISR(PCINT2_vect){
	//SNOOZE PRESSED
	if((PIND & (1 << PD3)) == 0){ //Snooze pressed
		if((PORTB & (1 << PD5)) != 0){//If timer is currently running
			counter = -30000; //30 seconds
		}
	}

	//TIMESET PRESSED
	if((PIND & (1 << PD2)) == 0){ //Set time is pressed
		_delay_ms(500); //bouncing
		displayState++;
		displayChanged = 1;
		if(displayState == 6){ //wrap
			displayState = 0;
			//save alarm values
		  eeprom_update_byte((void *) 7, alarmHourTens);
		  eeprom_update_byte((void *) 8, alarmHourOnes);
		  eeprom_update_byte((void *) 9, alarmMinuteTens);
		  eeprom_update_byte((void *) 10, alarmMinuteOnes);
		}
	}
}

/*Change on PORTB (Encoder)*/
ISR(PCINT0_vect){
	//ENCODER TURNED
	char add = 0; //whether to add or subtract
	a = PINB & (1 << PB3);
	b = PINB & (1 << PB4);
	// State transitions
	if (old_state == 0) {
	    if (a) {
		new_state = 1;
		add = 1;
	    }
	    else if (b) {
		new_state = 2;
		add = -1;
	    }
	}
	else if (old_state == 1) {
	    if (!a) {
		new_state = 0;
		add = -1;
	    }
	    else if (b) {
		new_state = 3;
		add = 1;
	    }
	}
	else if (old_state == 2) {
	    if (a) {
		new_state = 3;
		add = -1;
	    }
	    else if (!b) {
		new_state = 0;
		add = 1;
	    }
	}
	else {   // old_state = 3
	    if (!a) {
		new_state = 2;
		add = 1;
	    }
	    else if (!b) {
		new_state = 1;
		add = -1;
	    }
	}
	if(displayState == 1){ // hour
		if(add == -1){
			if(hourOnes == 0){ //10->9
				hourTens--;
				hourOnes = 9;
			}else if(hourOnes == 1 && hourTens == 0){ //01->24
				hourTens = 2;
				hourOnes = 4;
			}else{
				hourOnes+=add;
			}
		}else{
			hourOnes+=add;
			if(hourOnes == 10){ //09->10
				hourTens++;
				hourOnes = 0;
			}else if(hourOnes == 5 && hourTens == 2){ //24->01
				hourTens = 0;
				hourOnes = 1;
			}
		}
	}else if(displayState == 2){ //minutes
		if(add == -1){
			if(minuteOnes == 0){//00->59
				minuteOnes = 9;
				if(minuteTens == 0){
					minuteTens = 5;
				}else{
					minuteTens--;
				}
			}else{
				minuteOnes+=add;	
			}
		}else{
			minuteOnes+=add;
			if(minuteOnes == 10){//60->0
				minuteOnes = 0;
				if(minuteTens == 5){
					minuteTens = 0;
				}else{
					minuteTens+=add;
				}
			}
		}
	}else if(displayState == 3){//date
		if(add == -1){//because unsigned char
			if(day == 0){
				day = 6;
			}else{
				day+=add;
			}
		}else{//must be pos or 0
			day+=add;
			if(day == 7){
				day = 0;
			}
		}
	}else if(displayState == 4){//alarm hour
		if(add == -1){
			if(alarmHourOnes == 0){ //10->9
				alarmHourTens--;
				alarmHourOnes = 9;
			}else if(alarmHourOnes == 1 && alarmHourTens == 0){ //01->24
				alarmHourTens = 2;
				alarmHourOnes = 4;
			}else{
				alarmHourOnes+=add;
			}
		}else{
			alarmHourOnes+=add;
			if(alarmHourOnes == 10){ //09->10
				alarmHourTens++;
				alarmHourOnes = 0;
			}else if(alarmHourOnes == 5 && alarmHourTens == 2){ //24->01
				alarmHourTens = 0;
				alarmHourOnes = 1;
			}
		}
	}else if(displayState == 5){ //alarm minute
		if(add == -1){
			if(alarmMinuteOnes == 0){//00->59
				alarmMinuteOnes = 9;
				if(alarmMinuteTens == 0){
					alarmMinuteTens = 5;
				}else{
					alarmMinuteTens--;
				}
			}else{
				alarmMinuteOnes+=add;	
			}
		}else{
			alarmMinuteOnes+=add;
			if(alarmMinuteOnes == 10){//60->0
				alarmMinuteOnes = 0;
				if(alarmMinuteTens == 5){
					alarmMinuteTens = 0;
				}else{
					alarmMinuteTens+=add;
				}
			}
		}
	}
	if(displayState != 0){
		encoderChanged = 1;
	}
	old_state = new_state;
}

#endif