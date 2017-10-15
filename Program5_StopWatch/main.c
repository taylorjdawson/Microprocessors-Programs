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
#define STOPPED (0)
#define RUNNING (1)
#define RESET (2)
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

enum DISPLAY_TIMES
	{
		MAIN,
		LAP_0,
		LAP_1,
		LAP_2,
		NUM_DISPLAY_TIMES
	};
uint64_t display_time[NUM_DISPLAY_TIMES];
//-----------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//-----------------------------------------------------------------------------
static void zero_out_times();

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
		BUTTONS,
		NUM_TIMERS
	};
	
	uint8_t timer_state = RESET;
	uint64_t current_time;
	uint64_t delta;
	uint64_t event_time[NUM_TIMERS];

	uint64_t display;
	// Initialize timers
	current_time = timer_get();
	for (int i = 0; i < NUM_TIMERS; i++)
	{
		event_time[i] = current_time;
	}
	display_time[MAIN] = 0;
	display_time[LAP_0] = 0;
	display_time[LAP_1] = 0;
	display_time[LAP_2] = 0;
	//snprintf(mystring, sizeof(mystring), "Delta %s ", "Hit");	
	//usart_print(mystring);
    while (1) 
    {
		// Start the adc conversion. When the conversion is complete it will trigger the adc interrupt
		ADCSRA |= 1<<ADSC;
		
		// The current seven segment display states: Main Timer, Lap 1, 2, or 3
		uint8_t display_state = (int) adc_val / 256;
		
		current_time = timer_get();
		
		// Check SEVEN_SEG timer
		delta = current_time - event_time[SEVEN_SEG];
		if (delta >= UPDATE_SEVEN_SEG)
		{
			seven_seg_write(display_time[display_state]);
			event_time[SEVEN_SEG] = current_time;
		}
		
		// Check LIGHT_FADE timer
		delta = current_time - event_time[LIGHT_FADE];
		if (delta >= 25) 
		{
			light_fader(display_state);
			event_time[LIGHT_FADE] = current_time;
		}
		
		// Check BUTTONS timer
		delta = current_time - event_time[BUTTONS];
		if (delta >= 500)
		{	
			//Concatenate the output of these two function forming a 2 bit value.
			switch((buttons_get(0) << 1) | buttons_get(1))
			{
				case 0b00: // No buttons pressed
					break;
				case 0b01: // PB1 pressed, lap time
					display_time[LAP_2] = display_time[LAP_1];
					display_time[LAP_1] = display_time[LAP_0];
					display_time[LAP_0] += 1;				

					break;
				case 0b10: // PB0 pressed, so start if stopped and stop if started.
						timer_state = (timer_state == RUNNING) ? STOPPED : RUNNING;		
					break;
				case 0b11: // PB0 & PB1 pressed, so reset
					zero_out_times ();
					timer_state = STOPPED;
					break;
			}

			event_time[BUTTONS] = current_time;
		}
		
		// Check STOP_WATCH timer
		delta = current_time - event_time[STOP_WATCH];
		if (delta >= 100) 
		{
			display_time[MAIN] += timer_state != STOPPED? 1 : 0;
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
static void zero_out_times()
{
	for (int i = 0; i < NUM_DISPLAY_TIMES; i++)
	{
		display_time[i] = 0;
	}
}
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