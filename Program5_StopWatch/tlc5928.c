//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "tlc5928.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------
#define DDR_SPI	DDRB
#define DD_SCK	DDB5
#define DD_MOSI	DDB3
#define DD_MISO	DDB4
//------------------------------------------------------------------------------
//     ___      __   ___  __   ___  ___  __
//      |  \ / |__) |__  |  \ |__  |__  /__`
//      |   |  |    |___ |__/ |___ |    .__/
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//                __          __        ___  __
//     \  /  /\  |__) |  /\  |__) |    |__  /__`
//      \/  /~~\ |  \ | /~~\ |__) |___ |___ .__/
//
//------------------------------------------------------------------------------
static uint8_t busy = 0;
static uint8_t byte_0 = 0;
static uint8_t byte_1 = 0;
static uint8_t byte_state = 0;
static uint8_t spi_state = 0;
//------------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//------------------------------------------------------------------------------

//==============================================================================

void tlc5928_init()
{
		// Set up MOSI - PB3, SCK - PB5
		//PORTB &= ~ ((1 << 3) | (1 << 5));
		//DDRB  |=  ((1 << 3) | (1 << 5));
		
		//DDRB  |=  (1 << 2);
		
		//DDR_SPI |= (1<<DD_MOSI) | (1<<DD_SCK);
		
		// Set up the SPI as a Master, 250 KHz, Phase/Polarirty 00, order-MSB first
		//SPCR |= (1<<SPIE) | (1<<SPE) | (0<<DORD)| (1<<MSTR) | (1<<SPR1);
		
		DDRB |= (1<<2)|(1<<3)|(1<<5);    // SCK, MOSI and SS as outputs
		DDRB &= ~(1<<4);                 // MISO as input
		
		SPCR |= (1<<MSTR);               // Set as Master
		SPCR |= (0<<SPR0)|(1<<SPR1);     // divided clock by 128
		SPCR |= (1<<SPIE);               // Enable SPI Interrupt    
		SPCR |= (1<<SPE);                // Enable SPI
		
		// Set anodes high
		//DDRC = 0x3E;
		
		// Set blank to 1
		DDRD |= (1 << 7);
}

//==============================================================================

void tlc5928_write(uint8_t led, uint8_t hex)
{
	if (!busy) {
		byte_0 = hex;
		byte_1 = led;
	
		byte_state = 0;
		SPDR = byte_0;
		
		
		PORTC |=   (1 << 5);
		_delay_us(1);
		PORTC &= ~ (1 << 5);
	
		spi_state = 0;
		
		return (0);
	}
	else
		return(1);
}
//------------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//      __                  __        __        __
//     /  `  /\  |    |    |__)  /\  /  ` |__/ /__`
//     \__, /~~\ |___ |___ |__) /~~\ \__, |  \ .__/
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//        __   __  , __
//     | /__` |__)  /__`   
//     | .__/ |  \  .__/
//
//------------------------------------------------------------------------------
ISR(SPI_STC_vect)
{
	switch (byte_state)
	{
		case 0:
			byte_state++;
			SPDR = byte_1;
			break;
		case 1:
			byte_state = 0;
			//Pulse the latch
			PORTC |=   (1 << 5);
			PORTC &= ~ (1 << 5);
			break;
	}
}