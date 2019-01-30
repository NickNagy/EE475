/*
 * File:   main.c
 * Author: Nick Nagy
 *
 * Created on January 26, 2019, 2:54 PM
 */


#include <xc.h>
#include "config.h"
#include "pic18.h"
#include "pic18f452.h"
#include "stdio.h"
#include "constants.h"

#define _XTAL_FREQ 4000000
#pragma config WDT=OFF
#define DESIRED_BR 9600

// globals, helps free up space by putting samples om the heap
int samples[256];
unsigned char counter;

void main(void) {

	// spec page 170
	SYNC = 0;
	BRGH = 1;

	counter = 0;
	unsigned int baud_rate = convert_baud_rate();

	// may need to convert to binary representation?
	SPBRG = baud_rate; // convert_baud_rate();
					   // SSPADD = 9600;
					   // http://www.microcontrollerboard.com/pic_serial_communication.html
	TRISB = 0;
	PORTB = 0;
	// TXSTA = 0b00100010; // determining settings for the transmitter
	// RCSTA = 0b10010000; // determining settings for the receiver
	SSPSTAT[7] = 1; // disable slew rate

					// configuration for A/D converters
	ADCON0bits.CHS = 0b000;
	ADCON1bits.ADFM = 1; // 1 = right-justified, 0 = left-justified
	ADCON1bits.ADCS2 = 1;
	ADCON1bits.PCFG = 0b1001;

	clearTX();

	while (1) {
		// measure
		if (TXREG != 0) {
			while (ADCON0bits.GO_NOT_DONE); // wait til done reading
			samples[counter] = (ADRESH << 8) + ADRESL;// High or low??
			if (counter == 255) {
				TXREG = 0;
			}
			counter = (counter + 1) % 255;
		}
		//TXREG++;
		//while(!TRMT);
		//while(!RCIF);
		//PORTB=RCREG;
		__delay_ms(1);
	}
	return;
}

unsigned int convert_baud_rate() {
	unsigned char factor;
	if (BRGH) {
		factor = 16;
	}
	else {
		factor = 64;
	}
	return int(_XTAL_FREQ / (factor*(DESIRED_BR)) - 1);
}

void clearTX() {
	TXREG = 0x0;
}
