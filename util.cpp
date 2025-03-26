/* @(#) $Id: util.cpp 156 2024-10-19 23:26:34Z leres $ (XSE) */

#include <stdint.h>
#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "sdlogger.h"

#include "eeprom.h"
#include "serial.h"
#include "util.h"
#include "version.h"

/* Globals */
uint8_t *heapptr, *stackptr;
uint8_t boot_mcusr;

/*
 * This function places the current value of the heap and stack
 * pointers in the variables. You can call it from any place in
 * your code and save the data for outputting or displaying later.
 * This allows you to check at different parts of your program flow.
 * The stack pointer starts at the top of RAM and grows downwards.
 * The heap pointer starts just above the static variables etc. and
 * grows upwards. SP should always be larger than HP or you'll be
 * in big trouble! The smaller the gap, the more careful you need
 * to be. Julian Gall 6-Feb-2009.
 */
void
check_mem(void)
{
	// use stackptr temporarily
	stackptr = (uint8_t *)malloc(4);

	// save value of heap pointer
	heapptr = stackptr;

	// free up the memory again (sets stackptr to 0)
	free(stackptr);

	// save value of stack pointer
	stackptr = (uint8_t *)(SP);
}

void
hello(void)
{
	PRINTF("sdlogger " VERSION ", " STR(__AVR_DEVICE_NAME__) MHZ_STR
	    ", uart0 %lu baud, UART0_SIZE=" STR(UART0_SIZE) "\n", eeprom.speed);
}

/* Output blank delimited hex codes */
void
prhex(const __FlashStringHelper *msg, const u_char *cp, int n)
{
	int i;

	if (msg != NULL)
		serial_putstr(FV(msg));
	for (i = 0; i < n; ++i, ++cp) {
		if (i > 0)
			serial_putchar(' ');
		PRINTF("%02X", *cp & 0xff);
	}
	serial_nl();
}

/* Print seconds and milliseconds since boot and a blank */
void
prts(void)
{
	u_long ms;

	ms = millis();
	PRINTF("%lu.%03lu ", ms / 1000, ms % 1000);
}

/* Decode the MCU bootup status register */
void
prmcusr(char *cp, int8_t size)
{
	int8_t n;

	*cp = '\0';
	n = 0;
	if ((boot_mcusr & _BV(PORF)) != 0) {
		strlcpy_P(cp, PSTR(" power-on"), size);
		n = strlen(cp);
		size -= n;
		cp += n;
	}
	if ((boot_mcusr & _BV(EXTRF)) != 0) {
		strlcpy_P(cp, PSTR(" reset"), size);
		n = strlen(cp);
		size -= n;
		cp += n;
	}
	if ((boot_mcusr & _BV(BORF)) != 0) {
		strlcpy_P(cp, PSTR(" brown-out"), size);
		n = strlen(cp);
		size -= n;
		cp += n;
	}
	if ((boot_mcusr & _BV(WDRF)) != 0) {
		strlcpy_P(cp, PSTR(" watchdog"), size);
		n = strlen(cp);
		size -= n;
		cp += n;
	}
#ifdef JTRF
	if ((boot_mcusr & _BV(JTRF)) != 0) {
		strlcpy_P(cp, PSTR(" JTAG"), size);
		n = strlen(cp);
		size -= n;
		cp += n;
	}
#endif
}

/*
 * Reset using the watchdog
 *
 * Note: call wdt_enable() from setup() or wdt_reset() in loop()
 * when using this
 *
 */
void
reset(void)
{
	/* Disable interrupts */
	cli();

	/* Enable watchdog using the shortest time value */
	wdt_enable(WDTO_15MS);

	/* Wait for the end */
	for (;;)
		continue;
}

/* u_long to string with arbitrary base */
static const char digits[37] PROGMEM = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define MAX_BASE ((int8_t)(sizeof(digits) - 1))

int8_t
ui2str(uint16_t uv, char *buf, int8_t size, int8_t base)
{
	int8_t i, cc;
	uint16_t tuv;
	char ch, *p, *p2;

	if (base < 2 || base > MAX_BASE) {
		buf[0] = '\0';
		return (-1);
	}

	/* Build backwards version of string */
	p = buf;
	cc = 0;
	do {
		if (cc >= size - 1) {
			buf[0] = '\0';
			return (-1);
		}
		tuv = uv / base;
		i = uv % base;
		uv = tuv;
		*p++ = pgm_read_byte(digits + i);
		++cc;
	} while (uv > 0);
	*p = '\0';

	/* Now reverse it in place */
	p = buf;
	p2 = buf + cc - 1;
	for (i = cc / 2; i > 0; --i) {
		ch = *p;
		*p++ = *p2;
		*p2-- = ch;
	}

	return (cc);
}
