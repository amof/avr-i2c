/************************************************************************
Title:    Interrupt I2C library with receive/transmit circular buffers
Author:   Julien Delvaux
Software: Atmel Studio 7
Hardware: AVR 8-Bits, tested with ATmega1284P and ATmega88PA-PU
License:  GNU General Public License 3
Usage:    see Doxygen manual



LICENSE:
	Copyright (C) 2015 Julien Delvaux

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

    
************************************************************************/

/** 
 *  @defgroup avr-i2c I2C Library
 *  @code #include <twi.h> @endcode
 * 
 *  @brief Interrupt I2C library using the built-in I2C with transmit and receive circular buffers. 
 *
 *  This library can be used to transmit and receive data through the built in I2C. 
 *
 *  @author Julien Delvaux <delvaux.ju@gmail.com>
 */


#ifndef TWI_H_
#define TWI_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
#error "This library requires AVR-GCC 3.4 or later, update to newer AVR-GCC compiler !"
#endif

/************************************************************************/
/* Constants and macros                                                 */
/************************************************************************/

/* TWI Mode */
//#define TWI_MASTER_ENABLED
#define TWI_SLAVE_ENABLED

/** @brief  TWI Baudrate Expression
 *  @param  xtalCpu  system clock in Mhz, e.g. 4000000L for 4Mhz          
 *  @param  SCL frequency in kHz, e.g. 100, 400    
 */
#define TWI_FREQ_SELECT(xtalCpu, freq)  (xtalCpu/(2UL*freq*1000))-8UL //Refer to Datasheet to see calculus

/* Set size of receive and transmit buffers */

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE 64 /**< Size of the circular receive buffer, must be power of 2 */
#endif

#ifndef TWI_TX_BUFFER_SIZE
#define TWI_TX_BUFFER_SIZE 64 /**< Size of the circular transmit buffer, must be power of 2 */
#endif

/************************************************************************/
/* Functions prototype                                                  */
/************************************************************************/

/**
   @brief   Initialize TWI in Master and set SCL frequency
   @param   Specify SCL frequency using macro TWI_FREQ_SELECT()
   @return  none
*/
extern void twi_master_init(uint8_t scl_frequency);

/**
 *  @brief   Put string to ringbuffer for transmitting via I2C & start transmission
 *			 Stop when nothing more to transmit
 *  @param   s string to be transmitted
 *  @return  none
 */
extern void twi_master_transmit(uint8_t slaveAddress, const char *s);

/**
 *  @brief   Read x bytes from the slave
 *  @param   slaveAddress is the address of the slave
 *  @param   numberOfBytes to read from the slave
 *  @return  none
 */
extern void twi_master_read(uint8_t slaveAddress, uint8_t numberOfBytes);

/**
 *  @brief   Initialize SPI in Slave Mode
 *  @param   address is the address of the slave
 *  @param   ?
 *  @return  none
*/
extern void twi_slave_init(uint8_t address, uint8_t address_mask);

/**
 *  @brief   Allow to set TWI_statusReg.lastTransOK to TRUE by matching the number
 *			 of bytes expected with the one received.
 *  @param   address is the address of the slave
 *  @param   numberOfBytes to expect
 *  @return  none
*/
extern void twi_slave_receive(uint8_t numberOfBytes);

/**
 *  @brief   transmit string to I2C and ACK the I2C communication
 *  @param   s string to be transmitted
 *  @return  none
 */
extern void twi_slave_transmit(const char *s);

/**
 *  @brief   Get received byte from ringbuffer
 *
 * Returns in the lower byte the received character and in the 
 * higher byte the last receive error.
 *
 *  @return  byte:  received byte from ringbuffer
 */
extern uint8_t twi_getc(void);

/**
 *  @brief   Return number of bytes waiting in the receive buffer
 *  @return  bytes waiting in the receive buffer
 */
extern uint8_t twi_available(void);

/**
 *  @brief   Flush bytes waiting in receive buffer
 */
extern void twi_flush(void);
		
#endif /* TWI_H_ */