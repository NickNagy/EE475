/*
 * File:   main.c
 * Author: Nick Nagy
 *
 * Created on January 26, 2019, 2:54 PM
 */

#include <xc.h>
//#include "config.h"
//#include "pic18.h"
//#include "pic18f452.h"
#include "configv16.h"
#include "pic16f877a.h"
#include "stdio.h"
#include "constants.h"
#include "optfft.h"
#include "spi.h"

// temporarily keeping local, b/c not recognized when stored in constants.h:
#define _XTAL_FREQ 4000000
#define DESIRED_BR 9600
#pragma WDT = OFF
#pragma JTAG = OFF
#define COMM_FREQ 1
#define N 32

// globals, helps free up space by putting samples om the heap
int samples[N];
int imaginary[N] = {0};
int counter = 0;
int singleSample;
int baud_rate;
char mode;
char lineSkip[] = "\n";

// for debugging purposes
#define MODE 0

unsigned int convert_baud_rate() {
	unsigned long factor;
	if (TXSTAbits.BRGH) {
		factor = 16;
        SSPCON1bits.SSPM = 0b0001;
	}
	else {
		factor = 64;
        SSPCON1bits.SSPM = 0b0010;
	}
	return (unsigned int)((unsigned long)_XTAL_FREQ / (factor*(DESIRED_BR)) - 1);
}

char getRX(){
    while(PIR1bits.RCIF==0);
    RCIF = 0;
    return(RCREG);
}

void writeTX(char value){
    while(TXIF==0);
    TXIF = 0;
    TXREG = value;
}

void writeVal(char val) {
    writeTX(val);
    writeTX(lineSkip[0]);
    writeTX(lineSkip[1]);
}

void writeLine(char line[]){
    for (char i = 0; line[i]!= 0; i++){
        writeTX(line[i]);
    }
    writeTX(lineSkip[0]);
    writeTX(lineSkip[1]);
}

void clearTX() {
	writeTX(0);
}

void main(void) {    
    
    TXSTAbits.SYNC = 0; // 0 = asynchronous
    TXSTAbits.BRGH = 1; // used for baud rate calculation
    TXSTAbits.TX9 = 0;
    TXSTAbits.TXEN = 1; // enable transmit
    RCSTAbits.RX9 = 0;
    RCSTAbits.SPEN = 1; // enable RX and TX as serial
    RCSTAbits.CREN = 1; // continuous receive

    // SPI stuff
    SSPCON1bits.CKP = 0; // 0: idle clk state = low level
    SSPSTATbits.SMP = 0; // 0: middle, 1: end
    SSPSTATbits.CKE = 1; // which clock edge? (set opposite on slave)
    SSPSTATbits.SMP = 1; // disable slew rate

	TRISC = 0x80;
    // TRISC3 = 0; <- may be the same thing as above??
    SPBRG = convert_baud_rate();
					   
	TRISB = 0;
	PORTB = 0;
    
	// configuration for A/D converters
	ADCON0bits.CHS = 0b000;
    ADCON0bits.ADCS1 = 0;
    ADCON0bits.ADCS0 = 1;
	ADCON1bits.ADFM = 1; // 1 = right-justified, 0 = left-justified
	ADCON1bits.ADCS2 = 1;
	ADCON1bits.PCFG = 0b1111;

    writeLine("Initializing...");

	while (1) {
		clearTX(); // should TX send nothing b/w measurements??
        mode = getRX();
        if (mode > 0) {
            ADCON0bits.GO_nDONE = 1;
			while (ADCON0bits.GO_nDONE); // wait til done reading
			singleSample = (ADRESH << 8) + ADRESL;
            if (mode == COMM_FREQ) {
                samples[counter] = singleSample;
                if (counter == (N-1)) {
                    int frequency = optfft(samples, imaginary);
                    writeVal((char)frequency);
                }
                counter = (counter + 1) % N;
            } else {
                writeVal((char)singleSample);
            }
        }
		__delay_ms(5);
	}
	return;
}
