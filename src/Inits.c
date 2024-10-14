#define __AVR_ATMEGA8A__
#include <avr/io.h>

void TIMER0_Init (void)
{
	TIMSK = (1<<TOIE0);
	TCCR0 = ( (1<<CS01) | (1<<CS00) );
}

void TIMER1_Init (void)
{
	TCCR1A = ( (1<<COM1B1) | (1<<WGM11) | (1<<WGM10) ); // WGM11 WGM 10
	TCCR1B = ((1<<WGM12) | (1<<CS10) ); 
	OCR1B = 100;
	//TIMSK |= (1<<OCIE1A);
	//TIMSK |= (1<<TOIE1);
	//OCR1A = 62499;
	//TCCR1B |= (1<<CS12);
}

void GPIO_Init (void)
{
	DDRB = 0xFF;
	PORTB = 0x00;
	PORTB |= ( (1<<PINB1) | (1<<PINB0) );
	
	DDRC = 0x00;
	DDRC |= ( (1<<PINC4) | (1<<PINC5) ); // Software SPI output ports (CS & MOSI) + 'Toggle' LED & 'Start' LED (PC0-PC1);
	PORTC = 0x00;
	PORTC = 0x00;
	
	DDRD = ( (1<<PIND2) | (1<<PIND3) | (1<<PIND4) | (1<<PIND5) ); // PD2-PD5 is control outputs for collectors
	PORTD = 0x00;
}

void TIMER2_Init (void)
{
	TCCR2 |= ( (1<<CS21) | (1<<CS20) | (1<<WGM21) );
	TIMSK |= (1<<OCIE2);
	OCR2 = 200;
}
void SPI_Init (void)
{
	SPCR = ( (1<<SPE) | (1<<MSTR) | (1<<DORD) | (1<<SPR1) | (1<<SPR0) ); // (CPOL & SPHA = 0 ), LSB will transmitted first (DORD)
}

void Comparator_Init (void)
{
	ACSR |= (1<<ACIS1);
}
