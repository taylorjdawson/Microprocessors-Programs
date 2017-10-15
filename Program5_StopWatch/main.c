#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include "timer.h"
#include "seven_seg_ctrl.h"
#include "adc.h"
#include "usart.h"
#include "tlc5928.h"
#include "led_fade_ctrl.h"
#include "pwm.h"
#include "led.h"
#include "buttons.h"
//-----------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//-----------------------------------------------------------------------------
#define UPDATE_SEVEN_SEG (5)
#define MAX_STRING_SIZE (255)
#define _BV(bit) (1 << (bit))
//-----------------------------------------------------------------------------
//     ___      __   ___  __   ___  ___  __
//      |  \ / |__) |__  |  \ |__  |__  /__`
//      |   |  |    |___ |__/ |___ |    .__/
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//                __          __        ___  __
//     \  /  /\  |__) |  /\  |__) |    |__  /__`
//      \/  /~~\ |  \ | /~~\ |__) |___ |___ .__/
//
//-----------------------------------------------------------------------------
volatile static uint32_t stop_watch_time = 0;
volatile static uint16_t adc_val = 0;
//-----------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//-----------------------------------------------------------------------------

//=============================================================================

int main(void)
{   
	char mystring[MAX_STRING_SIZE];
	ADMUX |= _BV(REFS0); // Set ADC reference to AVCC
	ADCSRA |= _BV(ADEN) | _BV(ADIE) | _BV(ADSC) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
	timer_init();
	tlc5928_init();
	usart_init();
	led_init();
	pwm_init();
	light_fader_init();
	buttons_init();
	
	// enable interrupts
	sei();
	
enum TIMERS
	{
		SEVEN_SEG,
		LIGHT_FADE,
		STOP_WATCH,
		TIMER_D,
		NUM_TIMERS
	};
	
	uint64_t current_time;
	uint64_t delta;
	uint64_t event_time[NUM_TIMERS];

	// Initialize timers
	current_time = timer_get();
	for (int i = 0; i < NUM_TIMERS; i++)
	{
		event_time[i] = current_time;
	}
	//snprintf(mystring, sizeof(mystring), "Delta %s ", "Hit");	
	//usart_print(mystring);
    while (1) 
    {
		// Start the adc conversion. When the conversion is complete it will trigger the adc interrup
		ADCSRA |= 1<<ADSC;
		
		current_time = timer_get();
		
		// Check SEVEN_SEG timer
		delta = current_time - event_time[SEVEN_SEG];
		if (delta >= UPDATE_SEVEN_SEG)
		{
			seven_seg_write(stop_watch_time);
			event_time[SEVEN_SEG] = current_time;
		}
		
		// Check LIGHT_FADE timer
		delta = current_time - event_time[LIGHT_FADE];
		if (delta >= 25) 
		{
			light_fader((int) adc_val / 256);
			event_time[LIGHT_FADE] = current_time;
		}
		
		// Check STOP_WATCH timer
		delta = current_time - event_time[STOP_WATCH];
		if (delta >= 100) 
		{
			stop_watch_time++;
			event_time[STOP_WATCH] = current_time;
		}
    }
}

//-----------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//        __   __   __
//     | /__` |__) /__`
//     | .__/ |  \ .__/
//
//-----------------------------------------------------------------------------
// ISR for getting the converted adc value
ISR(ADC_vect)
{
 
	adc_val = ADC;			// Output ADCH to PortD
	ADCSRA |= 1<<ADSC;		// Start Conversion
}