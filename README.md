avr-i2c
===================

### 1. Description

This library is compatible with Atmega1284P and Atmega88PA-PU families.

### 2. Hardware supported

This library has been tested with :

 - Atmega1284P @8MHz -> 100kHz & 400kHz
 - Atmega88PA-PU @8MHz -> 100kHz & 400kHz

### 3. Memory used

**Reference** : Atmega1284P blank solution 

    Program Memory Usage 	:	164 bytes   0,1 % Ful
    Data Memory Usage 	    :	0 bytes   	0,0 % Full

**Reference** : Atmega88PA-PU blank solution 

	Program Memory Usage 	:	76 bytes   0,9 % Full
	Data Memory Usage 		:	0 bytes    0,0 % Full

**Test** : Atmega1284P in Master
	
	Program Memory Usage 	:	700 bytes   0,5 % Full
	Data Memory Usage 		:	135 bytes   0,8 % Full


**Test** : Atmega88PA-PU in Slave

	Program Memory Usage 	:	340 bytes   4,2 % Full
	Data Memory Usage 		:	134 bytes   13,1 % Full


### 4. Roadmap

 - Better memory usage
 - Multi-master gestion