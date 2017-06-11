/*************************************************************************
	
	I2C library by Julien Delvaux

*************************************************************************/

#include "twi.h"

/************************************************************************/
/* Constants and macros                                                 */
/************************************************************************/

/* size of RX/TX buffers */
#define TWI_RX_BUFFER_MASK ( TWI_RX_BUFFER_SIZE - 1)
#define TWI_TX_BUFFER_MASK ( TWI_RX_BUFFER_SIZE - 1)

#if ( TWI_RX_BUFFER_SIZE & TWI_RX_BUFFER_MASK )
#error RX buffer size is not a power of 2
#endif
#if ( TWI_RX_BUFFER_SIZE & TWI_TX_BUFFER_MASK )
#error TX buffer size is not a power of 2
#endif

/************************************************************************/
/* TWI constants                                                        */
/************************************************************************/

#define TRUE	1
#define FALSE	0

#define TWI_STATE_WAIT		0
#define TWI_STATE_SEND		1
#define TWI_STATE_REQUEST	2

#define TWI_readAtNextOp_FALSE		0
#define TWI_readAtNextOp_TRUE		1

/*  Bit and byte definitions */

#define TWI_READ_BIT  0   // Bit position for R/W bit in "address byte".
#define TWI_ADR_BITS  1   // Bit position for LSB of the slave address bits in the init byte.
#define TWI_GEN_BIT   0   // Bit position for LSB of the general call bit in the init byte.

/*  TWI State codes (TWSR) */

// General TWI Master status codes                      
#define TWI_START                  0x08  // START has been transmitted  
#define TWI_REP_START              0x10  // Repeated START has been transmitted
#define TWI_ARB_LOST               0x38  // Arbitration lost in SLA+W or data bytes
										 //						SLA+R or NACK bit
// TWI Master Transmitter status codes                      
#define TWI_MTX_ADR_ACK            0x18  // SLA+W has been transmitted and ACK received
#define TWI_MTX_ADR_NACK           0x20  // SLA+W has been transmitted and NACK received 
#define TWI_MTX_DATA_ACK           0x28  // Data byte has been transmitted and ACK received
#define TWI_MTX_DATA_NACK          0x30  // Data byte has been transmitted and NACK received

// TWI Slave Transmitter status codes
#define TWI_STX_ADR_ACK            0xA8  // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0  // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK           0xB8  // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK          0xC0  // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received 

// TWI Master Receiver status codes  
#define TWI_MRX_ADR_ACK            0x40  // SLA+R has been transmitted and ACK received
#define TWI_MRX_ADR_NACK           0x48  // SLA+R has been transmitted and NACK received
#define TWI_MRX_DATA_ACK           0x50  // Data byte has been received and ACK transmitted
#define TWI_MRX_DATA_NACK          0x58  // Data byte has been received and NACK transmitted

// TWI Slave Receiver status codes
#define TWI_SRX_ADR_ACK            0x60  // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK            0x70  // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78  // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK       0x80  // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK      0x88  // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK       0x90  // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK      0x98  // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART       0xA0  // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE               0xF8  // No relevant state information available; TWINT = “0”
#define TWI_BUS_ERROR              0x00  // Bus error due to an illegal START or STOP condition

// TWI TWCR shortcuts
#define TWSTART		TWCR =	(1<<TWEN)					|(1<<TWIE)|(1<<TWINT)										|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)		|(0<<TWWC)
							//TWI Interface enabled		|Enable TWI Interrupt and clear the flag to send byte		|Initiate a START condition
#define TWACK		TWCR =	(1<<TWEN)					|(1<<TWIE)|(1<<TWINT)										|(1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)		|(0<<TWWC)
							//TWI Interface enabled		|Enable TWI Interrupt and clear the flag to send byte		|Send ACK after next reception
#define TWNACK		TWCR =	(1<<TWEN)					|(1<<TWIE)|(1<<TWINT)										|(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)		|(0<<TWWC)
							//TWI Interface enabled		|Enable TWI Interrupt and clear the flag to read next byte	|Send NACK after reception
#define TWSTOP		TWCR =	(1<<TWEN)					|(0<<TWIE)|(1<<TWINT)										|(0<<TWEA)|(0<<TWSTA)|(1<<TWSTO)		|(0<<TWWC)
							//TWI Interface enabled		|Disable TWI Interrupt and clear the flag					|Initiate a STOP condition
#define TWRESTART	TWCR =	(1<<TWEN)					|(0<<TWIE)|(1<<TWINT)										|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)		|(0<<TWWC)
							//TWI Interface enabled		|Disable TWI Interrupt and clear the flag					|Initiate a (RE)START condition.	
#define TWRELEASE	TWCR =	(1<<TWEN)					|(1<<TWIE)|(1<<TWINT)										|(0<<TWEA)|(1<<TWSTA)|(0<<TWSTO)		|(0<<TWWC)
							//TWI Interface enabled		|Enable TWI Interrupt and clear the flag					|Initiate a (RE)START condition.

/************************************************************************/
/* Global variable                                                      */
/************************************************************************/
volatile struct TWI_statusReg TWI_statusReg;
static volatile uint8_t TWI_TxBuf[TWI_RX_BUFFER_SIZE];
static volatile uint8_t TWI_RxBuf[TWI_RX_BUFFER_SIZE];
static volatile uint8_t TWI_TxHead;
static volatile uint8_t TWI_TxTail;
static volatile uint8_t TWI_RxHead;
static volatile uint8_t TWI_RxTail;

static volatile uint8_t TWI_bytesRequest;		// Number of bytes requested

#if defined (TWI_MASTER_ENABLED) && defined(TWI_SLAVE_ENABLED)
	#error The multimaster mode of TWI is not yet implement. Do not hesitate to implement it, then submit a pull request ! Thx
#elif defined(TWI_MASTER_ENABLED)
#elif defined(TWI_SLAVE_ENABLED)
#endif

/************************************************************************/
/* Private functions                                                    */
/************************************************************************/

/**
 *  @brief   Put byte to ringbuffer for transmitting via I2C
 *  @param   data byte to be transmitted
 *  @return  none
 */
void twi_putc(uint8_t data);

/**
 *  @brief   Put string to ring buffer for transmitting via I2C
 *
 *  The string is buffered by the I2C library in a circular buffer
 *  and one character at a time is transmitted to the I2C using interrupts.
 *  Blocks if it can not write the whole string into the circular buffer.
 * 
 *  @param   s string to be transmitted
 *  @return  none
 */
void twi_puts(const char *s );


/*************************************************************************
Function: TWI interrupt
Purpose:  called when the TWI event has occurred
**************************************************************************/
ISR(TWI_vect){

	uint16_t tmphead=0;
	uint16_t tmptail=0;
			
	switch (TWSR)
	{
		/*MASTER*/
		#ifdef TWI_MASTER_ENABLED
			// START has been transmitted
			case TWI_START:
		
			// Repeated START has been transmitted
			case TWI_REP_START:
				
			/* Transmit process */
			//1. SLA+W has been transmitted and ACK received
			case TWI_MTX_ADR_ACK:
		
			//2. Data byte has been transmitted and ACK received
			case TWI_MTX_DATA_ACK:
			if(TWI_TxTail != TWI_TxHead) {
				// calculate and store new buffer index
				tmptail = (TWI_TxTail + 1) & TWI_TX_BUFFER_MASK;
				TWI_TxTail = tmptail;

				TWDR = TWI_TxBuf[tmptail];
				TWNACK;
			}else if(TWI_statusReg.readAtNextOp==TWI_readAtNextOp_TRUE){
				TWRESTART;
			}else{
				TWSTOP;
			}
			break;
		
			/*READ PROCESS*/
			//1. SLA+R has been transmitted and ACK received
			case TWI_MRX_ADR_ACK:
			if(TWI_bytesRequest==0){
				TWNACK;
			}else{
				TWI_bytesRequest--;
				TWACK;
			}
			break;
		
			//2. Data byte has been received and send ACK to continue or NACK to stop
			case TWI_MRX_DATA_ACK:
			tmphead = ( TWI_RxHead + 1) & TWI_RX_BUFFER_MASK;
			// store new index
			TWI_RxHead = tmphead;
			// store received data in buffer
			TWI_RxBuf[tmphead] = TWDR; // Detect the last byte to ACK it.
			if (TWI_bytesRequest>0) {
				TWI_bytesRequest--;
				TWACK;
			} else{
				TWNACK; // Send NACK after next reception
			}
			break;
			//3. Data byte has been received and NACK transmitted
			case TWI_MRX_DATA_NACK:
			tmphead = ( TWI_RxHead + 1) & TWI_RX_BUFFER_MASK;
			if ( tmphead != TWI_RxTail ) {
				// store new index
				TWI_RxHead = tmphead;
				// store received data in buffer
				TWI_RxBuf[tmphead] = TWDR;
				} else {
				// error: receive buffer overflow

			}
			TWI_statusReg.lastTransOK = TRUE;  // Set status bits to completed successfully.
			TWSTOP;
			break;
		
		/*SLAVE*/
		#elif defined(TWI_SLAVE_ENABLED)
			/*RECEIVE DATAS*/
			//1. Previously addressed with own SLA+W; data has been received; ACK has been returned
			case TWI_SRX_ADR_ACK:
			TWACK; // Reset the TWI Interrupt to wait for a new event.
			break;
			
			//2. Previously addressed with general call; data has been received; ACK has been returned
			case TWI_SRX_ADR_DATA_ACK:
			tmphead = ( TWI_RxHead + 1) & TWI_RX_BUFFER_MASK;
			if(tmphead != TWI_RxTail){
				TWI_RxHead = tmphead;
				TWI_RxBuf[tmphead] = TWDR;
			}else{
				//buffer overflow
			}
			TWI_statusReg.lastTransOK = TRUE;  // Set flag to transmission successful.
			TWACK; // Reset the TWI Interrupt to wait for a new event.
			break;
		
			/*SEND DATAS*/
			//1. Own SLA+R has been received; ACK has been returned
			case TWI_STX_ADR_ACK:
			//case TWI_STX_ADR_ACK_M_ARB_LOST: // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
		
			//2. Data byte in TWDR has been transmitted; ACK has been received
			case TWI_STX_DATA_ACK:
			if ( TWI_TxHead != TWI_TxTail) {
				// calculate and store new buffer index
				tmptail = (TWI_TxTail + 1) & TWI_TX_BUFFER_MASK;
				TWI_TxTail = tmptail;
				TWDR = TWI_TxBuf[tmptail];
				TWACK;
			}
			break;
		
			//3. Data byte in TWDR has been transmitted; NACK has been received.
			case TWI_STX_DATA_NACK:
			if (TWI_TxHead == TWI_TxTail) // Have we transceived all expected data?
			{
				TWI_statusReg.lastTransOK = TRUE;		// Set status bits to completed successfully.
			}
			else                          // Master has sent a NACK before all data where sent.
			{
				//TWI_state = TWSR;						// Store TWI State as errormessage.
			}
			TWACK;
			break;
		
			/*STOP CONDITION*/
			// A STOP condition or repeated START condition has been received while still addressed as Slave
			case TWI_SRX_STOP_RESTART:
			// Enter not addressed mode and listen to address match
			TWACK;
			break;
		
		#endif
		
		/*Error occurs*/
		
		// Arbitration lost
		case TWI_ARB_LOST:
		TWRELEASE;
		break;
		
		/*MASTER*/
		#ifdef TWI_MASTER_ENABLED
			// SLA+W has been transmitted and NACK received
			case TWI_MTX_ADR_NACK:
		
			// SLA+R has been transmitted and NACK received
			case TWI_MRX_ADR_NACK:
		
			// Data byte has been transmitted and NACK received
			case TWI_MTX_DATA_NACK:
		
		/*SLAVE*/
		#elif defined(TWI_SLAVE_ENABLED)
			// Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
			case TWI_SRX_ADR_DATA_NACK:
				
			// Previously addressed with general call; data has been received; NOT ACK has been returned
			case TWI_SRX_GEN_DATA_NACK:
				
			// Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received
			case TWI_STX_DATA_ACK_LAST_BYTE:
		
		#endif
		// Bus error due to an illegal START or STOP condition
		case TWI_BUS_ERROR:
		
		default:
		// TWI_state = TWSR;						// Store TWSR and automatically sets clears noErrors bit.
		// Reset TWI Interface
		TWCR = (1<<TWEN)|							// Enable TWI-interface and release TWI pins
		(0<<TWIE)|(0<<TWINT)|						// Disable Interrupt
		(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|			// No Signal requests
		(0<<TWWC);									//
	}
}


/*************************************************************************
Function: twi_busy()
Purpose:  Test if the TWI_ISR is busy transmitting.
Input:    none
Returns:  none
**************************************************************************/
uint8_t twi_busy(void)
{
	return (TWCR & (1<<TWIE));                  // IF TWI Interrupt is enabled then the Transceiver is busy
}

#ifdef TWI_MASTER_ENABLED
/*************************************************************************
Function: twi_master_init()
Purpose:  Initialize TWI in Master and set SCL frequency
Input:    Specify SCL frequency using macro TWI_FREQ_SELECT()
Returns:  none
**************************************************************************/
void twi_master_init(uint8_t scl_frequency)
{
	//reset state
	TWI_statusReg.currentState=TWI_STATE_WAIT;
	TWI_statusReg.lastTransOK=FALSE;
	TWI_statusReg.readAtNextOp=TWI_readAtNextOp_FALSE;
	
	TWBR = scl_frequency;						// Set bit rate register (Baudrate). Defined in header file.
	// TWSR = TWI_TWPS;							// Not used. Driver presumes prescaler to be 00.
	TWDR = 0xFF;								// Default content = SDA released.
	TWCR = (1<<TWEN)|							// Enable TWI-interface and release TWI pins.
	(0<<TWIE)|(0<<TWINT)|						// Disable Interrupt.
	(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|			// No Signal requests.
	(0<<TWWC);									//
}

/*************************************************************************
Function: twi_master_transmit()
Purpose:  transmit string to I2C and launch the I2C communication
Input:    address of the slave
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void twi_master_transmits(uint8_t slaveAddress, const char *s)
{
	// Wait until TWI is ready for next transmission.
	while (twi_busy());
	
	//Reset State TWI status
	TWI_statusReg.lastTransOK=FALSE;
	TWI_statusReg.currentState=TWI_STATE_SEND;
	
	// The first byte must always consist of General Call code or the TWI slave address.
	twi_putc((slaveAddress<<TWI_ADR_BITS) | (FALSE<<TWI_READ_BIT)); 
	
	// Stores data in buffer
	while (*s) {
		twi_putc(*s++);
	}		

	// Launch the transmission
	TWSTART;
}

/*************************************************************************
Function: twi_master_transmit()
Purpose:  transmit string to I2C and launch the I2C communication
Input:    address of the slave
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void twi_master_transmitc(uint8_t slaveAddress, uint8_t data)
{
	// Wait until TWI is ready for next transmission.
	while (twi_busy());
		
	//Reset State TWI status
	TWI_statusReg.lastTransOK=FALSE;
	TWI_statusReg.currentState=TWI_STATE_SEND;
	
	// The first byte must always consist of General Call code or the TWI slave address.
	twi_putc((slaveAddress<<TWI_ADR_BITS) | (FALSE<<TWI_READ_BIT));
	
	// Stores data in buffers
	twi_putc(data);

	// Launch the transmission
	TWSTART;
}

/*************************************************************************
Function: twi_master_read()
Purpose:  Read x bytes from the slave
Input:    slaveAdress is the I2C address of the slave
Input:    numberOfBytes that want to be read
Returns:  none
**************************************************************************/
void twi_master_read(uint8_t slaveAddress, uint8_t numberOfBytes)
{
	// Wait until TWI is ready for next transmission.
	while (twi_busy());
	
	//Reset State TWI status
	TWI_statusReg.lastTransOK=FALSE;
	TWI_statusReg.currentState=TWI_STATE_REQUEST;
	TWI_statusReg.readAtNextOp=TWI_readAtNextOp_FALSE;
	
	// last byte of data must be NACK
	TWI_bytesRequest = numberOfBytes-1;  // Number of data to transmit.
	
	// The first byte must always consist of General Call code or the TWI slave address.
	twi_putc((slaveAddress<<TWI_ADR_BITS) | (TRUE<<TWI_READ_BIT));
	
	// Launch the transmission
	TWSTART;
}

/*************************************************************************
Function: twi_master_transmit()
Purpose:  transmit string to I2C and launch the I2C communication
Input:    address of the slave
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void twi_master_transmitToRegister(uint8_t slaveAddress, uint8_t regAdress, uint8_t data)
{
	//Reset State TWI status
	TWI_statusReg.lastTransOK=FALSE;
	
	// The first byte must always consist of General Call code or the TWI slave address.
	twi_putc((slaveAddress<<TWI_ADR_BITS) | (FALSE<<TWI_READ_BIT));
	
	// Put register address
	twi_putc(regAdress);
	twi_putc(data);
	
	// Wait until TWI is ready for next transmission.
	while (twi_busy());

	// Launch the transmission
	TWSTART;
	
	while(TWI_statusReg.lastTransOK==FALSE && twi_busy()) ;
}

/*************************************************************************
Function: twi_master_readRegister()
Purpose:  Write the register address then read x bytes from the slave
Input:    slaveAdress is the I2C address of the slave
Input:    regAdress is the address of the register
Input:    numberOfBytes that want to be read
Returns:  none
**************************************************************************/
void twi_master_readRegister(uint8_t slaveAddress, uint8_t regAdress, uint8_t numberOfBytes)
{
	TWI_statusReg.readAtNextOp=TWI_readAtNextOp_TRUE;

	twi_master_transmitc(slaveAddress, regAdress);
	
	while(TWI_statusReg.lastTransOK==FALSE && twi_busy()) ;
				
	twi_master_read(slaveAddress, numberOfBytes);
	
	while(TWI_statusReg.lastTransOK==FALSE && twi_busy()) ;
		
	TWI_statusReg.readAtNextOp=TWI_readAtNextOp_FALSE;
	
}

#elif defined(TWI_SLAVE_ENABLED)
/*************************************************************************
Function: twi_slave_init()
Purpose:  Initialize TWI in Slave with address and mask
Input:    address
Input:    address_mask
Returns:  none
**************************************************************************/
void twi_slave_init(uint8_t address, uint8_t address_mask)
{
	TWAR = (address<<TWI_ADR_BITS)| (TRUE<<TWI_GEN_BIT);	// Set own TWI slave address. Accept TWI General Calls.
	TWAMR= address_mask<<TWI_ADR_BITS;						// Set own TWI slave address mask to enable respons on several addresses.
	TWCR = (1<<TWEN)|										// Enable TWI-interface and release TWI pins.
	(0<<TWIE)|(0<<TWINT)|									// Disable TWI Interrupt.
	(0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|						// Do not ACK on any requests, yet.
	(0<<TWWC);												//
}

/*************************************************************************
Function: twi_slave_receive()
Purpose:  Allow to set TWI_statusReg.lastTransOK to TRUE by matching the number
		  of bytes expected with the one received.
Input:    numberOfBytes
Returns:  none
**************************************************************************/
void twi_slave_receive(uint8_t numberOfBytes)
{
	while (twi_busy()) ;			// Wait until TWI is ready for next transmission.
	TWI_bytesRequest = numberOfBytes;
	
	TWACK;
}

/*************************************************************************
Function: twi_slave_transmit()
Purpose:  transmit string to I2C and ACK the I2C communication
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void twi_slave_transmit(const char *s)
{
	// Stores data in buffer
	while (*s) {
		twi_putc(*s++);
	}
	
	TWACK;
}
#endif

/*************************************************************************
Function: twi_getc()
Purpose:  return byte from ringbuffer
Returns:  byte:  received byte from ringbuffer
**************************************************************************/
uint8_t twi_getc(void)
{
	uint16_t tmptail;
	uint8_t data=0;

	if ( TWI_RxHead == TWI_RxTail ) {
		/* no data available */
	}
	else{
		/* calculate /store buffer index */
		tmptail = (TWI_RxTail + 1) & TWI_RX_BUFFER_MASK;
		TWI_RxTail = tmptail;

		/* get data from receive buffer */
		data = TWI_RxBuf[tmptail];
	}
	return data;
}

/*************************************************************************
Function: twi_putc()
Purpose:  write byte to ringbuffer for transmitting via twi
Input:    byte to be transmitted
Returns:  none
**************************************************************************/
void twi_putc(uint8_t data)
{
	uint16_t tmphead;

	tmphead  = (TWI_TxHead + 1) & TWI_TX_BUFFER_MASK;
	
	if (tmphead != TWI_TxTail){
		TWI_TxBuf[tmphead] = data;
		TWI_TxHead = tmphead;
	}
	
}

/*************************************************************************
Function: twi_puts()
Purpose:  transmit string to twi
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void twi_puts(const char *s )
{
	while (*s) {
		twi_putc(*s++);
	}

}

/*************************************************************************
Function: twi_available()
Purpose:  Determine the number of bytes waiting in the receive buffer
Input:    None
Returns:  Integer number of bytes in the receive buffer
**************************************************************************/
uint8_t twi_available(void)
{
	return (TWI_RX_BUFFER_SIZE + TWI_RxHead - TWI_RxTail) & TWI_RX_BUFFER_MASK;
}

/*************************************************************************
Function: twi_flush()
Purpose:  Flush bytes waiting the receive buffer. Actually ignores them.
Input:    None
Returns:  None
**************************************************************************/
void twi_flush(void)
{
	TWI_RxHead = TWI_RxTail;
}