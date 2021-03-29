/*
 * tiny13_ir.c
 *
 * Created: 2014-01-04 11:32:57
 *  Author: Olek
 
 */ 

//F_CPU=8 000 000 UL
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
//#include <stdlib.h>	//If use atoi();
#include "cpuclk.h"
#include "dd_rc5.h"

//PB2 INT0

/* Zmienne globalne z odebranymi danymi */
volatile unsigned char dd_rc5_dane_odebrane = 0;
volatile unsigned char dd_rc5_status = 0;
volatile unsigned char dd_rc5_last_toggle = 0;

int main(void)
{
	clock_prescaler_select(CLKDF_1); //9,6MHz
	DDRB = (1<<PORTB0)|(1<<PORTB1)|(1<<PORTB3);
	PORTB = 0;
	dd_rc5_ini();
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_IDLE);
	sei();
    while(1)
    {
        if (dd_rc5_status & DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU)
        {
			dd_rc5_status &= ~DD_RC5_STATUS_DANE_GOTOWE_DO_ODCZYTU;
			DD_RC5_WLACZ_DEKODOWANIE;
			//PORTB ^= (1<<PORTB0);
			if((dd_rc5_dane_odebrane == 0x0c)&(dd_rc5_last_toggle != (dd_rc5_status & DD_RC5_TOGGLE))) {PORTB ^= (1<<PORTB0); dd_rc5_last_toggle = (dd_rc5_status & DD_RC5_TOGGLE);}
			if((dd_rc5_dane_odebrane == 0x0e)&(dd_rc5_last_toggle != (dd_rc5_status & DD_RC5_TOGGLE))) {PORTB ^= (1<<PORTB1); dd_rc5_last_toggle = (dd_rc5_status & DD_RC5_TOGGLE);}
			EICRA &= ~(1<<ISC01);
			EICRA &= ~(1<<ISC00);
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_cpu();
		}
		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_cpu();
    }
}