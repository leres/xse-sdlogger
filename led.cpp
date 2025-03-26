/* @(#) $Id: led.cpp 148 2024-04-28 17:09:24Z leres $ (XSE) */

#if __has_include("local.h")
#include "local.h"
#endif

#include "sdlogger.h"

#include "led.h"
#include "status.h"

/* Locals */

/*
 * on/off times are 25 ms (LEDSTATEMS)
 * must be odd (including 0 end marker)
 * should total 40 (40 * 25ms = 1 second)
 */
static const int8_t PROGMEM ledrunstate[] = { 2, 38, 0 };
static const int8_t PROGMEM lederr2state[] = { 2, 4, 2, 32, 0 };
static const int8_t PROGMEM lederr3state[] = { 2, 4, 2, 4, 2, 26, 0 };
static const int8_t PROGMEM lederr4state[] = { 2, 4, 2, 4, 2, 4, 2, 20, 0 };
static const int8_t PROGMEM lederr5state[] =
    { 2, 4, 2, 4, 2, 4, 2, 4, 2, 14, 0 };

static const int8_t *ledp;		/* points to one of the state arrays */
static int8_t ledn;			/* index into led array */
static u_long last;			/* last millis() */
static int16_t left;			/* ms left until next state */
static boolean led;			/* current green LED state */
#define LEDSTATEMS 25

void
led_green(boolean on)
{
	digitalWrite(LED_GREEN, on);
}

void
led_mode(int8_t mode)
{
	switch (mode) {

	case LED_MODE_RUN:
		ledp = ledrunstate;
		break;

	case LED_MODE_ERR2:
		ledp = lederr2state;
		break;

	case LED_MODE_ERR3:
		ledp = lederr3state;
		break;

	case LED_MODE_ERR4:
		ledp = lederr4state;
		break;

	case LED_MODE_ERR5:
	default:
		ledp = lederr5state;
		break;
	}

	last = msec;
	led = 1;
	led_green(led);
	ledn = 0;
	left = ((int8_t)pgm_read_byte(ledp + ledn)) * LEDSTATEMS;
}

void
led_init(void)
{
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	led_mode(LED_MODE_RUN);
}

void
led_poll(void)
{
	int8_t v;

	left -= MILLIS_SUB(msec, last);
	if (left <= 0) {
		led = !led;
		led_green(led);
		++ledn;
		v = (int8_t)pgm_read_byte(ledp + ledn);
		if (v == 0) {
			ledn = 0;
			v = (int8_t)pgm_read_byte(ledp + ledn);
		}
		left = v * LEDSTATEMS;
	}
	last = msec;
}

void
led_red(boolean on)
{
	digitalWrite(LED_RED, on);
	status_set(on, STATUS_STATE_DIRTY);
}
