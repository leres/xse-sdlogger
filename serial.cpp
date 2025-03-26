/*
 * @(#) $Id: serial.cpp 156 2024-10-19 23:26:34Z leres $ (XSE)
 *
 * Copyright (c) 2008, 2009, 2011, 2012, 2013, 2014, 2015, 2017, 2020, 2021, 2022, 2024
 *	Craig Leres
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if __has_include("local.h")
#include "local.h"
#endif

#include <ctype.h>

#include <wiring_private.h>

#include <ctype.h>

#include "serial.h"

#ifndef IBUFSIZE
#define IBUFSIZE	64
#endif

/* Next position in circular input buffer */
#define IBUF_NEXTP(p) (((p) < ibuf + IBUFSIZE - 1) ? (p) + 1 : ibuf)

/* Fix up if pointer is past end of buffer */
#define IBUF_WRAP(p) (((p) < ibuf + IBUFSIZE) ? (p) : (p) - IBUFSIZE)

/* Previous position in circular input buffer */
#define IBUF_PREVP(p) ((p != ibuf) ? (p) - 1 : ibuf + IBUFSIZE - 1)

/* Locals */
static char *ip;			/* owned by main */
static char *igp;			/* owned by serial_getln() */
static char *iep;			/* owned by serial */
static char ibuf[IBUFSIZE];		/* input buffer */

/* speeds below 600 might not work */
static const uint32_t speeds[] PROGMEM = {
	110,
	134,
	150,
	200,
	300,
	600,
	1200,
	1800,
	2400,
	4800,
	7200,
	9600,
	14400,
	19200,
	28800,
	38400,
	57600,
	76800,
	115200,
	230400,
	460800,
	500000,
	921600,
	0,
};

NewSerialPort<1, 63, 63> NewSerial1;

void
serial_flush(void)
{
	NewSerial1.flush();
}

/* Returns true if a line was returned, else false */
boolean
serial_getln(char *buf, size_t size)
{
	int8_t i, j, len;
	char ch, *p, *cp, *cp2, *ip2, *igp2;
	static const char bsb[] PROGMEM = "\b \b";

	/* First bring igp up to date */
	while (igp != iep) {
		ch = *igp;
		p = IBUF_NEXTP(igp);
		if (isprint(ch)) {
			/* Echo */
			serial_putchar(ch);
			igp = p;
			continue;
		}
		switch (ch) {

		case '\r':
		case '\n':
			serial_nl();
			break;

		case '\b':
		case '\177':
			/* Backspace/delete: Eat it */
			/* Count the number of characters to eat */
			i = 1;
			ip2 = IBUF_PREVP(igp);

			/* Back up to the last character we're keeping */
			if (ip2 != IBUF_PREVP(ip) && *ip2 != '\r') {
				ip2 = IBUF_PREVP(ip2);
				++i;
			}

			/* Shift buffer */
			if (i > 1) {
				igp2 = igp;
				*igp2 = *ip2;
				while (ip2 != ip && *ip2 != '\r') {
					ip2 = IBUF_PREVP(ip2);
					igp2 = IBUF_PREVP(igp2);
					*igp2 = *ip2;
				}
				/* Back up cursor */
				serial_putstr(FV(bsb));
			}

			/* Update input pointer */
			ip = IBUF_WRAP(ip + i);
			break;

		case '\025':
			/* ^U */
			/* Count how far back we need to go */
			i = 1;
			ip2 = igp;
			while (ip2 != ip && *ip2 != '\r') {
				ip2 = IBUF_PREVP(ip2);
				++i;
			}

			/* Shift buffer */
			ip2 = igp - i;
			if (ip2 < ibuf)
				ip2 -= IBUFSIZE;
			igp2 = igp;
			for (j = i - 1; j > 0; --j) {
				ip2 = IBUF_PREVP(ip2);
				*igp2 = *ip2;
				igp2 = IBUF_PREVP(igp2);
				serial_putstr(FV(bsb));
			}

			/* Update input pointer, back up cursor */
			ip = IBUF_WRAP(ip + i);
			break;

		default:
			/* Invalid character: eat it */
			serial_putchar('\007');

			/* Count how far back need to shift buffer */
			i = 1;
			ip2 = igp;
			while (ip2 != ip && *ip2 != '\r') {
				ip2 = IBUF_PREVP(ip2);
				++i;
			}

			/* Shift buffer */
			ip2 = igp;
			igp2 = igp;
			while (i > 1) {
				ip2 = IBUF_PREVP(ip2);
				*igp2 = *ip2;
				igp2 = IBUF_PREVP(igp2);
				--i;
			}

			/* Update input pointer */
			ip = IBUF_WRAP(ip + i);
			break;
		}
		igp = p;
	}

	len = 0;
	cp2 = buf;
	for (cp = ip; cp != iep; cp = IBUF_NEXTP(cp)) {
		/* Don't overrun returned buffer */
		if (len < (int)size - 1) {
			*cp2++ = *cp;
			++len;
		}

		/* Return on CR, NL, or when we fill up the returned buffer */
		if (*cp == '\r' || *cp == '\n') {
			/* Update input pointer */
			ip = IBUF_WRAP(ip + len);

			/* Trim trailing junk */
			while (cp2 > buf && isspace(cp2[-1])) {
				--cp2;
				--len;
			}
			*cp2 = '\0';
			return (1);
		}
	}

	buf[0] = '\0';
	return (0);
}

/* Initialize Serial Interface */
void
serial_init(uint32_t speed)
{
	/* Initialize main serial */
	NewSerial1.begin(speed);

	/* avr-libc printf() setup */
	(void)fdevopen(serial_putc, NULL);

	/* Init pointers before we enable interrupts */
	ip = ibuf;
	iep = ip;
	igp = ip;
}

void
serial_nl(void)
{
	serial_putchar('\n');
}

/* Look to see if there are any new characters */
void
serial_poll(void)
{
	int ch;
	char *p;

	while (NewSerial1.available() > 0) {
		ch = NewSerial1.read();
		if (ch == '\0')
			continue;

		/*
		 * Allow erase characters, CR, or NL to fill second
		 * to last position
		 */
		p = IBUF_NEXTP(iep);
		if (IBUF_NEXTP(p) == ip && (ch != '\b' && ch != '\177' &&
		    ch != '\025' && ch != '\r' && ch != '\n')) {
			serial_putchar('\007');
			continue;
		}
		*iep = ch;
		iep = p;
	}
}

void
serial_prone(boolean first, const __FlashStringHelper *s1,
    const __FlashStringHelper *s2)
{
	if (first)
		serial_putstr(s1);
	else
		serial_putstr(s2);
}

/* fdevopen() helper */
int
serial_putc(char ch, FILE *f)
{
	return (serial_putchar(ch));
}

int
serial_putchar(char ch)
{
	if (ch == '\n')
		NewSerial1.write('\r');
	NewSerial1.write(ch);
	return (0);
}

/* Non-const version assumes string is stored in ram */
void
serial_putstr(char *s)
{
	char ch;

	while ((ch = *s++) != '\0')
		serial_putchar(ch);
}

/* Const version assumes string is stored in flash */
void
serial_putstr(const __FlashStringHelper *s)
{
	char ch;
	PGM_P p = reinterpret_cast<PGM_P>(s);

	while ((ch = pgm_read_byte(p++)) != '\0')
		serial_putchar(ch);
}

void
serial_prspeeds(void)
{
	boolean didany;
	int8_t col;
	uint32_t speed;
	const uint32_t *up;
	char buf[8];

	up = speeds;
	col = 0;
	didany = 0;
	for (;;) {
		speed = pgm_read_u32(up++);
		if (speed == 0)
			break;
		if (col > 68) {
			serial_nl();
			col = 0;
		}
		if (col == 0) {
			SERIAL_PUTSTR("    ");
			col += 4;
			didany = 0;
		}
		snprintf_P(buf, sizeof(buf), PSTR("%lu"), speed);
		if (didany) {
			serial_putchar(' ');
			++col;
		}
		didany = 1;
		serial_putstr(buf);
		col += strlen(buf);
	}
	if (col > 0)
		serial_nl();
}

/* Returns true of there are characters to be read */
boolean
serial_ready(void)
{
	return (ip != iep);
}

/* Returns true if speed is valid */
boolean
serial_speed(uint32_t speed)
{
	uint32_t speed2;
	const uint32_t *up;

	up = speeds;
	for (;;) {
		speed2 = pgm_read_u32(up++);
		if (speed2 == 0)
			break;
		if (speed2 == speed)
			return (1);
	}
	return (0);
}
