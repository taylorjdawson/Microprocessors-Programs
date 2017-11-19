 /*****************************************************************
*
* Name : Taylor Dawson
* Program : Program 4
* Class : ENGE 320
* Date : 2017-10-17
* Description : See pdf.
*
* =============================================================
* Program Grading Criteria
* =============================================================
* Stopwatch displays on the seven seg: (10) ____
* Display doesn’t ghost: (10) ____
* Stopwatch timer operates at the correct speed: (10) ____
* Start/Stop button works: (10) ____
* Stop time displayed for 2 seconds (if display set to a lap time): (10) ____
* Lap button works: (10) ____
* Lap time displayed for 2 seconds: (10) ____
* Potentiometer controls the Lap time display: (10) ____
* Lap times queue properly: (10) ____
* LEDs indicate the Lap time: (10) ___
* LEDs fade properly at 250ms: (20) ____
* Switch turns off the leds: (10) ____
* Serial port functions properly at 1MHz: (10) ____
* Serial start prints properly (10) ____
* Serial lap prints properly (10) ____
* Serial stop prints properly (10) ____
* Pressing “s” performs a start/stop: (10) ____
* Pressing “l” performs a lap: (10) ____
* Messages don’t cause system delay 1MHz: (10) ____
* =============================================================
*
* TOTAL (100)_________
*
* =============================================================
*****************************************************************/

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
#include "switch.h"
#include <math.h>
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
static volatile uint8_t rx_char= 0;

volatile static uint8_t timer_state = STOPPED;
enum DISPLAY_TIMES
	{
		MAIN,
		LAP_0,
		LAP_1,
		LAP_2,
		NUM_DISPLAY_TIMES
	};

uint64_t display_time[NUM_DISPLAY_TIMES];

bool button[2] = {false,false};
bool old_button[2] = {false, false};
//-----------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//-----------------------------------------------------------------------------
static void zero_out_times();
static uint8_t usart_get();
static uint8_t btns_get();
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
	
	//Initialize everything
	timer_init();
	tlc5928_init();
	usart_init();
	led_init();
	pwm_init();
	light_fader_init();
	buttons_init();
	switch_init();
	// enable interrupts
	sei();
	
	enum TIMERS
		{
			SEVEN_SEG,
			LIGHT_FADE,
			STOP_WATCH,
			BUTTONS,
			LAP_TIME,
			MAIN_TIME,
			NUM_TIMERS
		};
	

	uint8_t i;
	uint8_t lap_time_hold = 0;
	uint64_t lap_time_hold_timer = 0;
	uint8_t	 main_time_hold = 0;
	uint64_t main_time_hold_timer = 0;
	
	uint64_t current_time;
	uint64_t delta;
	uint64_t event_time[NUM_TIMERS];
	uint64_t display;
	uint64_t previous_time = 0;
	

	uint8_t display_state;

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
		// If we are in the lap_time_hold state then we want to be displaying the lap time rather than the main time. 
		display_state = lap_time_hold ? LAP_0 : (int) adc_val / 256;
		display_state = main_time_hold ? MAIN : display_state;
		current_time = timer_get();
		
		// Hold lap time for 2 seconds; the lap_time_hold_timer is set to 2 secs in the event that the laptime button or serial input was triggered.
		delta = current_time - event_time[LAP_TIME];
		if (delta >= lap_time_hold_timer)
		{
			// Reset lap_time_hold_timer back to 0
			lap_time_hold_timer = 0;
			lap_time_hold = 0;
			event_time[LAP_TIME] = current_time;
		}
		
		delta = current_time - event_time[MAIN_TIME];
		if (delta >= main_time_hold_timer)
		{
			// Reset main_time_hold_timer back to 0
			main_time_hold = 0;
			main_time_hold_timer = 0;
			event_time[MAIN_TIME] = current_time;
		}

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
			if (switch_get())
			{
				OCR0A = 0x00;
				OCR0B = 0x00;
				OCR1AL= 0x00;
			}
			else
			{
				light_fader(display_state);
			}
			event_time[LIGHT_FADE] = current_time;
		}
		
		// Check if buttons were pressed or the usart had something that we care about inputed.
		// The usart_get method returns a 1-hot encoding of the characters. This value is then ored with the buttons
		// enabling us to create 1 switch statement that responds to serial and button input.
		switch(btns_get(current_time) | usart_get())
				{
					case 0b00: // No buttons pressed
						break;
					case 0b01: // PB1 pressed, lap time
						if (timer_state == RUNNING){
							display_time[LAP_2] = display_time[LAP_1];
							display_time[LAP_1] = display_time[LAP_0];
							display_time[LAP_0] =  display_time[MAIN] - previous_time;
							previous_time = display_time[MAIN];
							
							int left_most = display_time[LAP_0] / (int) pow(10, 4) % 100;
							int middle = display_time[LAP_0] / (int) pow(10, 2) % 100;
							int right_most = display_time[LAP_0] / (int) pow(10, 0) % 100;
							
							
							sprintf(mystring, "lap time %02d:%02d:%02d \r\n", left_most, middle, right_most);
							usart_print(mystring);
							
							lap_time_hold = 1;
							lap_time_hold_timer = 20000;
						}
						break;
					case 0b10: // PB0 pressed, so start if stopped and stop if started.
							if (timer_state == RUNNING)
							{
								timer_state = STOPPED;
								
								display_time[LAP_2] = display_time[LAP_1];
								display_time[LAP_1] = display_time[LAP_0];
								display_time[LAP_0] =  display_time[MAIN] - previous_time;
								previous_time = display_time[MAIN];
								
								int left_most_lap = display_time[LAP_0] / (int) pow(10, 4) % 100;
								int middle_lap = display_time[LAP_0] / (int) pow(10, 2) % 100;
								int right_most_lap = display_time[LAP_0] / (int) pow(10, 0) % 100;
								
								int left_most_main = display_time[MAIN] / (int) pow(10, 4) % 100;
								int middle_main = display_time[MAIN] / (int) pow(10, 2) % 100;
								int right_most_main = display_time[MAIN] / (int) pow(10, 0) % 100;								
								
								sprintf(mystring, "lap time %02d:%02d:%02d\r\nfinal time: %02d:%02d:%02d\r\nStopwatch Completed\r\n \r\n",
								 left_most_lap, middle_lap, right_most_lap, left_most_main, middle_main, right_most_main);
								usart_print(mystring);
								
								// Used to keep the main time on the display after pressing stop
								main_time_hold = 1;
								main_time_hold_timer = 20000; // Amounts to 2 seconds
							}
							else {
								timer_state = RUNNING;
								usart_print("\r\nStopwatch Started\r\n");
							}
							
						break;
					case 0b11: // PB0 & PB1 pressed, so reset
						zero_out_times();
						timer_state = STOPPED;
						break;
				}
		// Check if switch is enabled
		if (switch_get()) 
		{
			led_set_value(0b00000000);
		}
		
		// Check STOP_WATCH timer
		delta = current_time - event_time[STOP_WATCH];
		if (delta >= 100) 
		{
			display_time[MAIN] += timer_state;
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

static uint8_t btns_get(uint64_t current_time)
{
	uint8_t button_0 = 0;
	uint8_t button_1 = 0;
	
	// See if the buttons have been pressed
	for (uint8_t i = 0; i < 2; i++)
	{
		button[i] = buttons_get_debounce(i, current_time / 10);
		// Just look for the falling edge
		if ((true == button[i]) && (false == old_button[i]))
		{
			button_0 = button[0];
			button_1 = button[1];
		}
		// Save the button val
		old_button[i] = button[i];
	}
	return (button_0 << 1) | ( button_1 );
}

static uint8_t usart_get()
{
	
	uint8_t retval;
	
	// This is 
	switch(rx_char)
	{
		case 's': retval = 0b10; break;
		case 'l': retval = 0b01; break;
		case 'r': retval = 0b11; break;
		default : retval = 0b00; break;
	}
	rx_char = 0;
	return (retval);

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

ISR(USART_RX_vect)
{
	rx_char = usart_read();
	
}