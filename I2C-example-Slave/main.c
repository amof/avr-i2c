/************************************************************************
Title:    Example I2C-slave
Brief:	  This example shows how to receive and send data with I2C
Author:   Julien Delvaux
Software: Atmel Studio 7
Hardware: AVR 8-Bits, tested with ATmega1284P and ATmega88PA-PU
License:  GNU General Public License 3


LICENSE:
	Copyright (C) 2015 Julien Delvaux

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
    
************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "I2C/twi.h"

#define slaveAddress 0x12


int main(void)
{
    uint8_t buf[15];
	uint8_t currentIndex=0;
	
	twi_slave_init(slaveAddress, slaveAddress);
	sei();
	
	twi_slave_receive(11);
	
    while (1) 
    {
		while(twi_available()>0){
			buf[currentIndex++]=twi_getc();
		}
		if(currentIndex>10){
			twi_slave_transmit(buf);
			currentIndex=0;
		}

    }
}

