#define __AVR_ATMEGA8A__
#include <avr/io.h>

#define LINEAR_PERCENT_LENGTH_ADDR 0
#define PERCRETA_FACTOR_INIT_ADDR 1 // 'Default' Init
#define PERCRETA_FACTOR_INIT2_ADDR 2 // 'Fast' Init
#define PERCRETA_FACTOR_INIT3_ADDR 3 // 'Slow' Init
#define PERCRETA_FACTOR_ADDR 4
#define ACCEPTABLE_RPM_DEVIATION_ADDR 5
#define N_dr_P_ADDR 6
#define INIT_PWM_VALUE_ADDR 7
#define CURVYCUTOFF_MULTIPLIER_ADDR 8
#define CURVYCUTOFF_MULTIPLIER_LOW_ADDR 9

uint8_t readByteFromAddr (uint8_t addr)
{
	while (EECR & (1<<EEWE));	
	EEAR = addr;
	EECR |= (1<<EERE);
	return EEDR;
}
