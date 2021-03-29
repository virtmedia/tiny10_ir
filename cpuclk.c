/*
 * cpuclk.c
 *
 * Created: 2014-02-13 00:00:40
 *  Author: Olek
 */ 

#include "cpuclk.h"
#include <util/atomic.h>

void clock_prescaler_select(uint8_t division_factor)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		CCP = 0xD8;
		CLKPSR = division_factor;
	}
}