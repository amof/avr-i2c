/************************************************************************
Title:    Example I2C-master
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

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "I2C/twi.h"

#define slaveAddress 0x12

int main(void)
{
    twi_master_init(TWI_FREQ_SELECT(F_CPU,400));
	sei();
	
	_delay_ms(25);
	twi_master_transmits(slaveAddress, "HELLO WORLD");
	_delay_ms(5);
	twi_master_read(slaveAddress, 11);
	
    while (1) 
    {
		;
    }
}

