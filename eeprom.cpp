/* @(#) $Id: eeprom.cpp 156 2024-10-19 23:26:34Z leres $ (XSE) */

#if __has_include("local.h")
#include "local.h"
#endif

#include "sdlogger.h"

#include <EEPROM.h>

#include "cmd.h"
#include "eeprom.h"
#include "serial.h"
#include "strings.h"

/* Globals */
struct eeprom eeprom;

void
eeprom_cmd(char *s)
{
	u_long uv;
	char *ep;

	switch (*s) {

	case 'r':
		/* report */
		eeprom_report();
		break;

	case 's':
		++s;
		while (*s == ' ')
			++s;
		uv = strtoul(s, &ep, 10);
		if (*ep != '\0' || !serial_speed(uv)) {
			serial_putstr(FV(msg_badvalue));
			SERIAL_PUTSTR("valid speeds:\n");
			serial_prspeeds();
			break;
		}
		if (eeprom.speed != uv) {
			eeprom.speed = uv;
			eeprom_write(1);
		}
		break;

	default:
		/* help */
		SERIAL_PUTSTR(
		    "'er'\treport\n"
		    "'es'\tspeed\n"
		    );
		break;
	}
}

void
eeprom_init(void)
{
	boolean didany;

	/* Read the eeprom, set defaults if uninitialized */
	didany = 0;
	eeprom_read();
	if (eeprom.debug == 0xff) {
		eeprom.debug = 0;
		didany = 1;
	}
	debug = eeprom.debug;
	if (eeprom.logseq == 0xffff) {
		eeprom.logseq = 0;
		didany = 1;
	}
	if (!serial_speed(eeprom.speed)) {
		eeprom.speed = UART0_BAUD;
		didany = 1;
	}
	if (didany)
		(void)eeprom_write(1);
}

void
eeprom_read(void)
{
	int i, ei;
	u_char *dp;

	dp = (u_char *)&eeprom;
	for (i = 0, ei = EEPROM_START; i < (int)sizeof(eeprom); ++i, ++ei)
		*dp++ = EEPROM.read(ei);
}

void
eeprom_report(void)
{
	/* Debugging */
	PRINTF("%5u logseq\n", eeprom.logseq);
	PRINTF("%5u debug\n", eeprom.debug);
	PRINTF("%5lu speed\n", eeprom.speed);
}

int8_t
eeprom_write(boolean verbose)
{
	int i, ei;
	u_char uc, *dp;
	int8_t didany, error;

	didany = 0;
	error = 0;
	dp = (u_char *)&eeprom;
	for (i = 0, ei = EEPROM_START; i < (int)sizeof(eeprom); ++i, ++ei) {
		/* Only write when necessary */
		uc = EEPROM.read(ei);
		if (uc != *dp) {
			EEPROM.write(ei, *dp);
			/* Trust but verify */
			uc = EEPROM.read(ei);
			if (uc != *dp)
				error = 1;
			didany = 1;
		}
		++dp;
	}
	if (error) {
		if (debug || verbose)
			serial_putstr(FV(msg_eepromfail));
		return (-1);
	}
	if (didany && verbose)
		SERIAL_PUTSTR("eeprom updated\n");
	return (didany);
}
