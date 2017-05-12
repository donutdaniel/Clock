#ifndef SERIAL_C
#define SERIAL_C

//set up RX TX
#define FOSC 16000000 // Clock Frequency
#define BAUD 9600 // Baud Rate
#define MYUBRR (FOSC/16/BAUD-1) // Value for UBRR0

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "serial.h"
#include "lcd.h"
#include "clock.h"

void init_serial(){
	UBRR0 = MYUBRR; // Set baud rate
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0); // Enable RX and TX
	UCSR0C = (3 << UCSZ00); // Asynchronized, no parity
 
	//turn on tri-state enable signal
	DDRC |= (1 << PC3); // set PC3 to output
	PORTC &= ~(1 << PC3); // enable buffer by outputting 0

	//enable interrupts for RX
	UCSR0B |= (1 << RXCIE0);
	//enable global interrupts
	sei();

	// Initialize global vars
	dataBuffer[0] = '+';
	dataBuffer[1] = 0x00;
	dataBuffer[2] = 0x00;
	dataBuffer[3] = 0x00;
	dataBuffer[4] = '\0';
	startFlag = 0;
	dataBufferSize = 0;
	validFlag = 0;
}

//Recieve ISR
ISR(USART_RX_vect){
	unsigned char ch = UDR0;

	//check for start
	if(ch == '#'){
		startFlag = 1;
		dataBufferSize = 0;
		validFlag = 0;
	}else if(ch == '$' && dataBufferSize >= 1){
		startFlag = 0;
		validFlag = 1;
		dataBufferSize = 0;
	}else{
		if(startFlag == 1){
			if(dataBufferSize == 0){ //must recieve a + or -
				if(ch == '-' || ch == '+'){
					dataBuffer[0] = ch;
					dataBufferSize++;
				}else{ //invalid, restart
					startFlag = 0;
				}
			}else if(dataBufferSize >= 1 && dataBufferSize <= 3){ //normal digits input
				if(ch >= '0' && ch <= '9'){
					dataBuffer[dataBufferSize] = ch - '0'; //input actual number (int)
					dataBufferSize++;				
				}else{ //bad input
					startFlag = 0;
					dataBufferSize = 0;
				}
			}else{ //inputting too many numbers, reset
				startFlag = 0;
				dataBufferSize = 0;
			}
		}
	}
}

#endif