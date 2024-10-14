#define __AVR_ATMEGA8A__
#define F_CPU 16000000UL

#include <avr/io.h>
#include "Inits.c"
#include "eeprom.c"
#include <avr/interrupt.h>
#include <util/delay.h>
// --------------MASKS OF CHARACTERS-----------------//
#define n0 0x03
#define n1 0x9F
#define n2 0x25
#define n3 0x0D
#define n4 0x99
#define n5 0x49
#define n6 0x41
#define n7 0x1F
#define n8 0x01
#define n9 0x19
//---------------INDICATION--------------------------//
volatile uint8_t VRAM[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
volatile uint8_t VRAM_Pointer = 0;
volatile uint8_t collectorCounter = 0x04;

#define SSPI_CS_PIN PINC5
#define SSPI_MOSI_PIN PINC4
//---------------RPM--------------------------------//
volatile uint8_t periodCaptured = 0;
volatile uint16_t CurrentRPMInteger = 0;
volatile uint16_t TargetRPMIngeter = 0;
volatile uint16_t rotations = 0;
volatile uint16_t rotationsBufCaptured = 0;
volatile uint8_t CompIters = 0;
volatile uint16_t PWM = 0;
volatile uint16_t RawRPMDeviation = 0;
volatile uint32_t LinearMiddleRPM = 0;

#define TimeTopLimit 240 // 4 Hours Max
#define RPMTopLimit 9999
#define RPMBottomLimit 200
#define CurvyCutOff_Multiplier_Base 50
//------------------------------------------
volatile uint8_t AcceptableRPMDeviation;
volatile uint8_t PercentLinearFactor;
volatile uint8_t LinearPercretaFactorInit;
volatile uint8_t LinearPercretaFactor;
volatile uint8_t N_dr_P;
volatile uint8_t InitPWMValue;
volatile uint8_t LinearPercretaFactor2;
volatile uint8_t LinearPercretaFactorInit2;
volatile uint8_t LinearPercretaFactorInit3;
volatile uint16_t CurvyCutOff;
volatile uint16_t CurvyCutOffLow;

//-------------------------------------------
volatile uint8_t droppedPeriods = 0;
volatile uint8_t InWork = 0; // 0 - Avaiting the input (target RPM, target Time), 1 - UI is 'locked', centrifuge in work
volatile uint8_t ToggleSpeedAndTime = 0; // 0 - Show target (if isInUse = 0) RPM or REAL (if isInUse = 1) RPM on rotor
volatile uint8_t clock = 0;
volatile uint16_t TargetTimeMins = 0;
volatile uint8_t seconds = 0;
volatile uint16_t timer2_Counter = 0; // 2500 ticks every second
volatile uint16_t LinearRPM_Target = 0;

//--------------------------------------------------//
void DecimalToMask (uint16_t valueDec)
{	
	uint8_t i = 3;
	uint8_t tmpBuf[4] = {};
	
		for (uint8_t k = 0; k<4; k++)
				{
					tmpBuf[i] = valueDec % 10;
					valueDec = valueDec / 10;			
					if (i>0) { i--; }
				}
					for (uint8_t e = 0; e<4; e++)
						{		
							switch (tmpBuf[i])
								{
									case 0: VRAM[i] = n0; break;
									case 1: VRAM[i] = n1; break;
									case 2: VRAM[i] = n2; break;
									case 3: VRAM[i] = n3; break;
									case 4: VRAM[i] = n4; break;
									case 5: VRAM[i] = n5; break;
									case 6: VRAM[i] = n6; break;
									case 7: VRAM[i] = n7; break;
									case 8: VRAM[i] = n8; break;
									case 9: VRAM[i] = n9; break;
									default: VRAM[i] = 0xFF; break;
								}
							i++;
						}
}
//--------------------------------------------------//

//--------------------------------------------------//
ISR (TIMER0_OVF_vect)
{
	PORTD &= ~(collectorCounter); // Turn off last-opened collector	
	
			if ( collectorCounter < 32 )
				{
	
					collectorCounter = collectorCounter<<1; // if yes -> 0b0000100 << 1 ; -> 0b00001000
					VRAM_Pointer++;
				}
				else
					{
						collectorCounter = 0x04; // set to zero point (DIG1) ('Zero Point' starts from PIND2)
						VRAM_Pointer = 0;
					}
	uint8_t	temp = 0;
	temp = VRAM[VRAM_Pointer];
	
	/* Software SPI WRITE implementation */
	for (uint8_t z = 0; z<8; z++)
	{
		if (temp & 0x01)
		{
			PORTC |= (1<<SSPI_MOSI_PIN);
		}
		else
		{
			PORTC &= ~(1<<SSPI_MOSI_PIN);
		}
		__asm__ volatile ("nop");
		__asm__ volatile ("nop");
		__asm__ volatile ("nop");
		__asm__ volatile ("nop");
		
		PORTC &= ~(1<<SSPI_CS_PIN);
	
		__asm__ volatile ("nop");
		__asm__ volatile ("nop");
		__asm__ volatile ("nop");
		__asm__ volatile ("nop");
	
		PORTC |= (1<<SSPI_CS_PIN);
		temp = temp>>1;
	}	
	PORTD |= collectorCounter; // Turn on last-opened collector		
}
//--------------------------------------------------//

//--------------------------------------------------//
ISR (ANA_COMP_vect)
{
		rotations++;
}
//--------------------------------------------------//

//--------------------------------------------------//
ISR (TIMER2_COMP_vect)
{
	if (timer2_Counter != 2500)
	{
		timer2_Counter++;
	}
	else
	{
	
	PORTB |= (1<<PINB1);
		
	if (PINC & (1<<PINC3))
		{
			if (ToggleSpeedAndTime)
			{
				ToggleSpeedAndTime=0;
				PORTB &= ~(1<<PINB1);
			}
			else
			{
				ToggleSpeedAndTime=1;
				PORTB &= ~(1<<PINB1);
			}
		}
		if (clock)
		{
			if (TargetTimeMins>=1)
			{
				seconds++;
				if (seconds==60)
				{
					TargetTimeMins-=1;
					seconds = 0;
				}
			}
		}
			if (PORTB & (1<<PINB0) && (clock) && (AcceptableRPMDeviation >= RawRPMDeviation) )
			{
					PORTB &= ~(1<<PINB0);
			}
			else
			{
				PORTB |= (1<<PINB0);
			}

	if (CompIters)
	{
		periodCaptured=1; // Set Flag
		CompIters = 0; // Drop Counter (Counter of Fronts (Fronts on Comparator Output) )
		ACSR &= ~(1<<ACIE); // Turn off Comparator Interrupts
		rotationsBufCaptured = rotations; // Copy
		rotations = 0;
	}
	
	if (!CompIters)
	{
		CompIters = 1; // Increment Counter
		ACSR |= (1<<ACIE); // Turn on Comparator Interrupts;
	}
		timer2_Counter=0;
		
	}
}
//--------------------------------------------------//

//--------------------------------------------------//
void getRpm (void)
{
	CurrentRPMInteger = rotationsBufCaptured * 60;
	periodCaptured = 0;
}
//--------------------------------------------------//

//--------------------------------------------------//
void RPMCorrection (void)
{
	uint16_t FactorP = 0;
	PWM = OCR1B;
		if ( LinearMiddleRPM < TargetRPMIngeter )
		{
			RawRPMDeviation = TargetRPMIngeter-LinearMiddleRPM;
		
			if ( LinearMiddleRPM < LinearRPM_Target )
			{
					if ( (LinearMiddleRPM <= CurvyCutOff) && (LinearMiddleRPM >= CurvyCutOffLow) ) // Middle 
					{
						FactorP = LinearPercretaFactorInit;
					}
					
					if ( (LinearMiddleRPM <= CurvyCutOff) && (LinearMiddleRPM <= CurvyCutOffLow) ) // Low
					{
						FactorP = LinearPercretaFactorInit3;
					}
					
					if ( (LinearMiddleRPM >= CurvyCutOff) && (LinearMiddleRPM >= CurvyCutOffLow) ) // High
					{
						FactorP = LinearPercretaFactorInit2;
					}
			}
			else
			{
				if ( RawRPMDeviation > AcceptableRPMDeviation )
				{
					FactorP = LinearPercretaFactor;
				}
			}
			
				if (droppedPeriods==N_dr_P && ( !(RawRPMDeviation < AcceptableRPMDeviation)) )
				{
					uint16_t summ = PWM+FactorP;
				if ( (summ) < 1023 )
				{
					OCR1B += FactorP;
				}
				else
				{
					while  (!( (summ) < 1023 ))
					{
						FactorP--;
						summ = PWM+FactorP;
					}
				OCR1B = summ;
				}
					droppedPeriods=0;
				}
		}
	//---------------------------------------------------//
	
		if ( (LinearMiddleRPM > TargetRPMIngeter) )
		{
			RawRPMDeviation = LinearMiddleRPM-TargetRPMIngeter;	
			if (TargetRPMIngeter == 0)
			{
				if (LinearMiddleRPM <= CurvyCutOffLow)
				{
					FactorP = LinearPercretaFactorInit3;
				}
				else
				{
					FactorP = LinearPercretaFactorInit;
				}
			}
			else
			{
				if (RawRPMDeviation > AcceptableRPMDeviation)
				{
					FactorP = LinearPercretaFactor;
				}
			}
				if (droppedPeriods==N_dr_P && ( !(RawRPMDeviation < AcceptableRPMDeviation)) )
				{
					if (PWM > FactorP)
					{
						OCR1B -= FactorP;
					}
					else
					{
						while ( !( PWM >= FactorP) )
						{
							FactorP--;
						}
						OCR1B -= FactorP;
					}
						droppedPeriods=0;
				}
		}
		
		if (ToggleSpeedAndTime)
		{
			DecimalToMask( (uint16_t)LinearMiddleRPM);
		}
		if (droppedPeriods==N_dr_P)
		{
			droppedPeriods = 0;
		}
}
//--------------------------------------------------//

//--------------------------------------------------//
void PreparingToStart (void)
{
	CurrentRPMInteger = RPMBottomLimit;
	uint8_t preparingPhrase = 0; // 0 = awaiting the target RPM value, 1 = awaiting the target TIME value
	
	// Taking target values (RPM, Time, awainting the approving of user choice) ----->
	while (InWork==0) // also, PINC3 = Toggle, PINC2 = VOL+, PINC1 = VOL-, PINC0 = START/STOP
	{
			// PHRASE 0 (RPM)
			while (preparingPhrase==0)
			{
				// --------------------> VOL+
				if (PINC & (1<<PINC2))
				{
					
					if ( (TargetRPMIngeter+100) <= RPMTopLimit)
					{
						TargetRPMIngeter+=100;
					}
					_delay_ms(400);
				    if (PINC & (1<<PINC2))
				    {
						if ( (TargetRPMIngeter+900) <= RPMTopLimit)
						{
							TargetRPMIngeter+=900;
						}
						DecimalToMask(TargetRPMIngeter);
						_delay_ms(1000);
					}
				}
				// VOL+ <-------------------
				
				// -------------------> VOL-
				if (PINC & (1<<PINC1))
				{
					if ( TargetRPMIngeter >= 100 )
					{
						TargetRPMIngeter-=100;
					}
					_delay_ms(400);
					if (PINC & (1<<PINC1))
					{
						if (TargetRPMIngeter >= 900)
						{
							TargetRPMIngeter-=900;
						}
							DecimalToMask(TargetRPMIngeter);
							_delay_ms(1000);
					
					}
				}
				// VOL- <--------------------
				
				DecimalToMask(TargetRPMIngeter);
				
				while (PINC & (1<<PINC3))
				{
					_delay_ms(3000);
					if (PINC & (1<<PINC3))
					{
						preparingPhrase = 0;
						TargetRPMIngeter = RPMBottomLimit;
					}
					while (PINC & (1<<PINC3))
					{};
				}
				
				while (PINC & (1<<PINC0)) // Approving the choice (target RPM)
				{
					preparingPhrase = 1;
					if (!(PINC & (1<<PINC0)))
					{
						break;
					}
				}
				
			}

			// PHRASE 1 (TIME)
			
			_delay_ms(500); // Just for c delay before LED switch
			PORTB &= ~(1<<PINB1); // 'Time' Status LED Turn-on
			while (preparingPhrase==1)
			{
				// --------------------> VOL+
				if (PINC & (1<<PINC2))
				{
					
					if ( (TargetTimeMins+1) <= TimeTopLimit)
					{
						TargetTimeMins+=1;
					}
					_delay_ms(400);
				    if (PINC & (1<<PINC2))
				    {
						if ( (TargetTimeMins+9) <= TimeTopLimit)
						{
							TargetTimeMins+=9;
						}
						DecimalToMask(TargetTimeMins);
						_delay_ms(1000);
					}
				}
				// VOL+ <-------------------
				
				// -------------------> VOL-
				if (PINC & (1<<PINC1))
				{
					if ( TargetTimeMins >= 1 )
					{
						TargetTimeMins-=1;
					}
					_delay_ms(400);
					if (PINC & (1<<PINC1))
					{
						if ( TargetTimeMins >= 9)
						{
							TargetTimeMins-=9;
						}
							DecimalToMask(TargetTimeMins);
							_delay_ms(1000);
					
					}
				}
				// VOL- <--------------------
				
				DecimalToMask(TargetTimeMins);
				
				while (PINC & (1<<PINC3))
				{
					_delay_ms(3000);
					if (PINC & (1<<PINC3))
					{
						preparingPhrase = 0;
						TargetTimeMins = 0;
					}
					while (PINC & (1<<PINC3))
					{};
				}
				
				while (PINC & (1<<PINC0) && (TargetTimeMins>=1)) // Approving the choice
				{
					_delay_ms(3000);
					if (PINC & (1<<PINC0))
					{
						PORTB |= (1<<PINB1);
						PORTB &= ~(1<<PINB0);
						InWork = 1;
						preparingPhrase=2;
						LinearRPM_Target = (TargetRPMIngeter / 100 * PercentLinearFactor);
						TIMER2_Init();
						break;
					}
				}
				
			}
	}
}
//--------------------------------------------------//

//--------------------------------------------------//
void LoadSettings (void)
{
	// Linear Curvy Length
	if ( (readByteFromAddr(LINEAR_PERCENT_LENGTH_ADDR) ) > 100 )
	{
		PercentLinearFactor = 100;
	}
	else
	{
		PercentLinearFactor = readByteFromAddr(LINEAR_PERCENT_LENGTH_ADDR);
	}
	//
	
	LinearPercretaFactor = readByteFromAddr(PERCRETA_FACTOR_ADDR);
	LinearPercretaFactorInit = readByteFromAddr(PERCRETA_FACTOR_INIT_ADDR);
	AcceptableRPMDeviation = readByteFromAddr(ACCEPTABLE_RPM_DEVIATION_ADDR);
	N_dr_P = readByteFromAddr(N_dr_P_ADDR);
	InitPWMValue = readByteFromAddr(INIT_PWM_VALUE_ADDR);
	
	LinearPercretaFactorInit2 = readByteFromAddr(PERCRETA_FACTOR_INIT2_ADDR);
	
	CurvyCutOff = (readByteFromAddr(CURVYCUTOFF_MULTIPLIER_ADDR) ) * CurvyCutOff_Multiplier_Base;
	CurvyCutOffLow = (readByteFromAddr(CURVYCUTOFF_MULTIPLIER_ADDR)) * (readByteFromAddr(CURVYCUTOFF_MULTIPLIER_LOW_ADDR));
	LinearPercretaFactorInit3 = readByteFromAddr(PERCRETA_FACTOR_INIT3_ADDR);
}
//--------------------------------------------------//

//--------------------------------------------------//
void MainAlgo (void) // Capture the target RPM before using clock (decrement mins-counter)
{
	PORTB |= (1<<PINB4); // Turn on Relay
	DDRB |= (1<<PINB2); // Rotor
	TIMER1_Init();
	OCR1B = InitPWMValue;
	while (InWork)
	{
		cli();
		if (periodCaptured)
		{
			getRpm();
			if (droppedPeriods!=N_dr_P)
			{
				droppedPeriods++;
				LinearMiddleRPM += CurrentRPMInteger;
			}
			else
			{
				LinearMiddleRPM = LinearMiddleRPM/N_dr_P;
				RPMCorrection();
				LinearMiddleRPM = 0;
			}
		}
		sei();
		
		if (!(ToggleSpeedAndTime))
		{
			DecimalToMask(TargetTimeMins);
		}
			
		if (AcceptableRPMDeviation >= RawRPMDeviation)
		{
			clock = 1;
		}
		else
		{
			clock = 0;
		}
		
		if (TargetTimeMins==0)
		{
			TargetRPMIngeter = 0;
		}
		if ( (CurrentRPMInteger <= 100) && (TargetTimeMins==0) )
		{
			clock = 0;
			PORTB |= (1<<PINB0);
			TCCR1B &= ~(1<<CS11);
			PORTB &= ~(1<<PINB2);
			DDRB &= ~(1<<PINB2);
			OCR1B = 0;
			PORTB &= ~(1<<PINB4);
			TargetRPMIngeter = RPMBottomLimit;
			TargetTimeMins = 0;
			LinearMiddleRPM = 0;
			InWork = 0;
		}
	}
}
//--------------------------------------------------//

//--------------------------------------------------//
int main (void)
{
	cli();
	
	GPIO_Init();
	TIMER0_Init();
	Comparator_Init();
	OCR1B = InitPWMValue;
	LoadSettings();
	TargetRPMIngeter = RPMBottomLimit;
	DecimalToMask(TargetRPMIngeter);
	
	sei();
	while(1)
	{
		switch (InWork)
		{
			case 0: PreparingToStart(); break; // Input the Target Time+RPM by user, approving the choice (manually by user), starting the engine, etc
			case 1: MainAlgo(); break;
			default: break;
		}
	}
}
//--------------------------------------------------//
