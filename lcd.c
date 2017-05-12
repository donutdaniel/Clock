/********************************************
*
*  Name: Hsiao Tung Ho
*  Section: 2pm
*  Assignment: LCD
*
********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "lcd.h"
void writenibble(unsigned char);

/*
  init_lcd - Configure the I/O ports and send the initialization commands
*/
void init_lcd()
{
    DDRD |= ((1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7)); // Set the DDR register bits for ports B and D
    DDRB |= ((1 << PB0) | (1 << PB1)); 

    //init. buttons for input
    DDRC &= ~((1 << PC2) | (1 << PC4));
    PORTC |= ((1 << PC2) | (1 << PC4)); //init pullup resistors

    _delay_ms(15);              // Delay at least 15ms

    writenibble(0x03);          // Use writenibble to send 0011
    _delay_ms(5);               // Delay at least 4msec

    writenibble(0x03);          // Use writenibble to send 0011
    _delay_us(120);             // Delay at least 100usec

    writenibble(0x03);          // Use writenibble to send 0011, no delay needed

    writenibble(0x02);          // Use writenibble to send 0010
    _delay_ms(2);               // Delay at least 2ms
    
    writecommand(0x28);         // Function Set: 4-bit interface, 2 lines'
    _delay_ms(2);

    writecommand(0x0f);         // Display and cursor on
    _delay_ms(2);
}

/*
  moveto - Move the cursor to the row (0 or 1) and column (0 to 15) specified
*/
void moveto(unsigned char row, unsigned char col)
{
  unsigned char dest = 0x00;
  if(row==1){
    dest = 0x40;
  }
  dest |= col;
  writecommand(0x80 + dest);
}

/*
  stringout - Write the string pointed to by "str" at the current position
*/
void stringout(char *str)
{
  int i;
  for(i = 0; str[i] != NULL; i++){
    writedata(str[i]);
  }
}

/*
  writecommand - Send the 8-bit byte "cmd" to the LCD command register
*/
void writecommand(unsigned char cmd)
{
  PORTB &= ~(1 << PB0);
  writenibble(cmd); //write upper 4 bits
  cmd = (cmd << 4); //shift cmd up 4 bits
  writenibble(cmd); //write lower 4 bits
  _delay_ms(2);
}

/*
  writedata - Send the 8-bit byte "dat" to the LCD data register
*/
void writedata(unsigned char dat)
{
  PORTB |= (1 << PB0);
  writenibble(dat);
  dat = (dat << 4);
  writenibble(dat);
  _delay_ms(2);
}

/*
  writenibble - Send four bits of the byte "lcdbits" to the LCD
*/
void writenibble(unsigned char lcdbits)
{
  PORTD &= (1 << PD0) | (1 << PD1) | (1 << PD2) | (1 << PD3); //clear PORTD[7:4]
  unsigned char mask = (lcdbits & ((1 << PD4) | (1 << PD5) | (1 << PD6) | (1 << PD7))) | (0x0f); //set PORTD[7:4] to upper 4 bits of lcdbits
  PORTD |= mask; //copy

  PORTB |= (1 << PB1);//enable 0 1 0 
  PORTB |= (1 << PB1);
  PORTB &= ~(1 << PB1);
}
