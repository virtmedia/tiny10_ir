/*
 * cpuclk.h
 *
 * Created: 2014-02-13 00:01:36
 *  Author: Olek
 */ 


#ifndef CPUCLK_H_
#define CPUCLK_H_

#include <util/atomic.h>

#define CLKDF_1  (0<<CLKPS3)|(0<<CLKPS2)|(0<<CLKPS1)|(0<<CLKPS0)
#define CLKDF_2  (0<<CLKPS3)|(0<<CLKPS2)|(0<<CLKPS1)|(1<<CLKPS0)
#define CLKDF_4  (0<<CLKPS3)|(0<<CLKPS2)|(1<<CLKPS1)|(0<<CLKPS0)
#define CLKDF_8  (0<<CLKPS3)|(0<<CLKPS2)|(1<<CLKPS1)|(1<<CLKPS0)
#define CLKDF_16 (0<<CLKPS3)|(1<<CLKPS2)|(0<<CLKPS1)|(0<<CLKPS0)
#define CLKDF_32 (0<<CLKPS3)|(1<<CLKPS2)|(0<<CLKPS1)|(1<<CLKPS0)
#define CLKDF_64 (0<<CLKPS3)|(1<<CLKPS2)|(1<<CLKPS1)|(0<<CLKPS0)
#define CLKDF_128 (0<<CLKPS3)|(1<<CLKPS2)|(1<<CLKPS1)|(1<<CLKPS0)
#define CLKDF_256 (1<<CLKPS3)|(0<<CLKPS2)|(0<<CLKPS1)|(0<<CLKPS0)

void clock_prescaler_select(uint8_t division_factor);  

#endif /* CPUCLK_H_ */