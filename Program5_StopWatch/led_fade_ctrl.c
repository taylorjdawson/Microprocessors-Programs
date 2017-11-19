//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------
#include "led_fade_ctrl.h"
#include "pwm.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------
#define RED (0)
#define YELLOW (1)
#define GREEN (2)
#define CYAN (3)
#define LIGHTS_OUT (4)
#define COMPLETE (0);
#define INCOMPLETE (1);
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
volatile static uint8_t dir = 0;
volatile static uint8_t transistion = 0;
static uint8_t current_state = 0;
//------------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//------------------------------------------------------------------------------
void light_fader(uint8_t adc_val);
void light_fader_init();
static uint8_t in_blue();
static uint8_t out_blue();
static uint8_t in_green();
static uint8_t out_green();
static uint8_t in_red();
static uint8_t out_red();
//------------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//------------------------------------------------------------------------------
void light_fader_init()
{
	OCR0A = 0x00;
	OCR0B = 0xF0;
	OCR1AL= 0x00;
}
//==============================================================================
void light_fader(uint8_t next_state)
{
	
	if (next_state != current_state && !transistion) {
		transistion = INCOMPLETE;
		dir = next_state > current_state;
	}
	
	if (transistion)
	{
			switch(current_state)
			{
				case RED:
					OCR0B = OCR0B? OCR0B: 0xF0;
					if(!in_green()) {
						current_state = YELLOW;
						transistion = COMPLETE;
					}
					break;
				case YELLOW:
					
					if(!dir) {
						OCR0A = OCR0A? OCR0A : 0xF0;
						if(!out_green()) {
							current_state = RED;
							OCR0A = 0x00; // Turn off Green LED
							transistion = COMPLETE;
						}
					}
					else {
						if(!out_red()) {
							current_state = GREEN;
							OCR0B = 0x00; //Turn off Red LED
							transistion = COMPLETE;
						}
					}
					break;
				case GREEN:
					if(!dir) {
						if(!in_red()) {
							current_state = YELLOW;
							transistion = COMPLETE;
						}
					}
					else {
						if(!in_blue()) {
							current_state = CYAN;
							transistion = COMPLETE;
						}
					}
					break;
				case CYAN:
					OCR1AL = OCR1AL? OCR1AL : 0xF0;
					if(!out_blue()) {
						current_state = GREEN; 
						OCR1AL = 0x00; //Turn off Blue LED
						transistion = COMPLETE;					
					}
					break;
			}
		}
}


//------------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//------------------------------------------------------------------------------

// Below are small routines for increasing the duty cycle of the specified color
static uint8_t in_blue()
{
	return OCR1AL++ <= 0xF0;
}

static uint8_t out_blue()
{
	return OCR1AL-- > 0x0A;
}

static uint8_t in_green()
{
	return OCR0A++ <= 0xF0;
}

static uint8_t out_green()
{
	return OCR0A-- > 0x0A;
}
static uint8_t in_red()
{
	return OCR0B++ <= 0xF0;
}
 
static uint8_t out_red()
{
	return OCR0B-- > 0x0A;
}
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
