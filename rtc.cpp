/*
 * @(#) $Id: rtc.cpp 160 2025-03-24 23:32:11Z leres $ (XSE)
 *
 * Copyright (c) 2012, 2015, 2017, 2021, 2022, 2024
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

/*
 * Maxim DS3231 I2C real-time clock
 */

#if __has_include("local.h")
#include "local.h"
#endif

#include <ctype.h>

#include <Wire.h>

#include "sdlogger.h"

#ifdef HAVE_EEPROM_RTC
#include "eeprom.h"
#endif
#include "rtc.h"
#include "serial.h"
#include "strings.h"
#include "util.h"

/* Globals */
struct rtc_time rtc_time = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Forwards */
static boolean rtc_convert(void);
static boolean rtc_idle(void);
static boolean rtc_read(uint8_t, uint8_t *, int8_t);
static boolean rtc_read2(uint8_t, uint8_t *, int8_t);
static boolean rtc_write(uint8_t, uint8_t *, int8_t);

#ifdef RTC_AGING
void
rtc_aging(int8_t offset)
{
	int8_t v;
	uint8_t uch;

	if (!rtc_read(RTC_OFFSET, &uch, 1)) {
		serial_putstr(FV(msg_no_rtc));
		return;
	}
	v = (int8_t)TWOSCOMPLEMENT(uch);
	if (offset != v) {
		uch = TWOSCOMPLEMENT(offset);
		if (!rtc_write(RTC_OFFSET, &uch, 1)) {
			serial_putstr(FV(msg_no_rtc));
			return;
		}
		if (rtc_read(RTC_OFFSET, &uch, 1)) {
			v = (int8_t)TWOSCOMPLEMENT(uch);
			PRINTF("rtc aging offset set to %d\n", v);
		}
	}
}
#endif

void
rtc_cmd(char *s)
{
	u_long uv;
	long v;
	int8_t i, units, hundreths;
	uint16_t usv1;
	char *ep;
	char ch;
	uint8_t uch;
	int8_t v1, v2, v3;
	struct rtc_time *rt;
	u_char buf[19];
	struct rtc_temp temp;

	switch (*s) {

	case '\0':
		/* Print the TOD */
		if (!rtc_pr(1)) {
			serial_putstr(FV(msg_no_rtc));
			break;
		}
		serial_nl();
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		/* Parse the time/date */
		uv = strtoul(s, &ep, 10);
		if (*ep != ':' && *ep != '-') {
			serial_putstr(FV(msg_badvalue));
			break;
		}
		/* Keep possible year */
		usv1 = uv;
		ch = *ep;
		s = ep + 1;

		uv = strtoul(s, &ep, 10);
		if (*ep != ch) {
			serial_putstr(FV(msg_badvalue));
			break;
		}
		v2 = uv;
		s = ep + 1;

		uv = strtoul(s, &ep, 10);
		if (*ep != '\0') {
			serial_putstr(FV(msg_badvalue));
			break;
		}
		v3 = uv;

		if (ch == ':') {
			/* Validate and set time */
			if (usv1 >= 24 || v2 >= 60 || v3 >= 60) {
				serial_putstr(FV(msg_badvalue));
				break;
			}
			v1 = usv1;
			PRINTF("parsed %02d:%02d:%02d\n", v1, v2, v3);
			if (!rtc_set(v1, v2, v3)) {
				serial_putstr(FV(msg_no_rtc));
				break;
			}
		} else if (ch == '-') {
			/* Validate and set date (clock chip limits) */
			if (usv1 < 1900 || usv1 > 2099 ||
			    v2 < 1 || v2 > 12 ||
			    v3 < 1 || v3 > 31) {
				serial_putstr(FV(msg_badvalue));
				break;
			}
			PRINTF("parsed %04d-%02d-%02d\n", usv1, v2, v3);
			if (!rtc_setdate(usv1, v2, v3)) {
				serial_putstr(FV(msg_no_rtc));
				break;
			}
		}

		/* Print the TOD */
		if (!rtc_pr(1))
			serial_putstr(FV(msg_no_rtc));
		else
			serial_nl();
		break;

	case '-':
	case '+':
		/* Parse and apply offset */
		ch = *s++;
		uv = strtoul(s, &ep, 10);
		if (*ep != '\0') {
			serial_putstr(FV(msg_badvalue));
			break;
		}
		if (uv > 24L * 3600L) {
			SERIAL_PUTSTR("Can't handle more than 1 day\n");
			break;
		}

		/* Hours */
		v1 = uv / 3600L;
		uv %= 3600L;

		/* Minutes and seconds */
		v2 = uv / 60L;
		v3 = uv % 60L;

		PRINTF("parsed %c%02d:%02d:%02d\n", ch, v1, v2, v3);
		if (ch == '-') {
			v1 = -v1;
			v2 = -v2;
			v3 = -v3;
		}

		if (!rtc_query()) {
			serial_putstr(FV(msg_no_rtc));
			return;
		}
		rt = &rtc_time;
		v1 += RTC2HOUR(rt);
		v2 += RTC2MIN(rt);
		v3 += RTC2SEC(rt);

		/* Adjust seconds */
		while (v3 > 60) {
			v3 -= 60;
			++v2;
		}
		while (v3 < -60) {
			v3 += 60;
			--v2;
		}

		/* Adjust minutes */
		while (v2 > 60) {
			v2 -= 60;
			++v1;
		}
		while (v2 < -60) {
			v2 += 60;
			--v1;
		}

		/* Keep track of delta days */
		i = 0;

		/* Adjust hours */
		while (v1 > 24) {
			v1 -= 24;
			++i;
		}
		while (v1 < -24) {
			v1 += 24;
			--i;
		}

		if (!rtc_set(v1, v2, v3)) {
			serial_putstr(FV(msg_no_rtc));
			break;
		}

		/* Too much work to automatically adjust (leap year, etc) */
		if (i != 0)
			PRINTF("WARNING: %d day lost\n", i);

		/* Print the TOD */
		if (!rtc_pr(1))
			serial_putstr(FV(msg_no_rtc));
		else
			serial_nl();
		break;

	case 'D':
		/* Dump everything */
		memset(buf, 0xff, sizeof(buf));
		if (!rtc_read(RTC_SECS, buf, sizeof(buf))) {
			serial_putstr(FV(msg_no_rtc));
			break;
		}
		SERIAL_PUTSTR("    ");
		for (i = 0; i <= 0x12; ++i)
			PRINTF(" %02X", i);
		serial_nl();
		PRHEX("rtc: ", buf, sizeof(buf));
		break;

#ifdef HAVE_EEPROM_RTC
	case 'I':
		++s;
		while (*s == ' ')
			++s;
		if (*s != '\0') {
			if (!parseuint8(s, &eeprom.rtc_setoffset, 1))
				break;
			if (eeprom_write(1) == 0)
				SERIAL_PUTSTR("no change\n");
			eeprom_read();
		}
		PRINTF("rtc_setoffset %u\n", eeprom.rtc_setoffset);
		break;
#endif

	case 'O':
		/* Optionally parse and apply aging offset then report */
		++s;
		while (*s == ' ')
			++s;
		if (isdigit(*s) || *s == '+' || *s == '-') {
			v = strtol(s, &ep, 10);
			if (*ep != '\0') {
				serial_putstr(FV(msg_badvalue));
				break;
			}
			if (v < -128 || v > 127) {
				SERIAL_PUTSTR("value must be +127/-128\n");
				break;
			}
			uch = TWOSCOMPLEMENT(v);

			/* Update aging offset */
			if (!rtc_write(RTC_OFFSET, &uch, 1)) {
				serial_putstr(FV(msg_no_rtc));
				break;
			}

#ifdef HAVE_EEPROM_RTC
			/* Update eeprom */
			eeprom.rtc_offset = v;
			(void)eeprom_write(1);
#endif

			/* Force a temperature conversion */
			if (!rtc_convert()) {
				serial_putstr(FV(msg_no_rtc));
				break;
			}
		}

		/* Now report the aging offset */
		if (!rtc_read(RTC_OFFSET, &uch, 1)) {
			serial_putstr(FV(msg_no_rtc));
			break;
		}

		/* Need cast to correctly recover sign bit */
		v = (int8_t)TWOSCOMPLEMENT(uch & 0xff);
		PRINTF("aging offset: %ld", v);

		/* Convert to ppm (multiple by 0.12) avoiding floating point */
		v = v * 12;
		units = v / 100;
		hundreths = v % 100;
		if (hundreths < 0)
			hundreths = -hundreths;
		PRINTF(", %d.%02d ppm", units, hundreths);
		PRINTF(", 0x%X\n", uch & 0xff);
		break;

	case 'S':
		/* Toggle 8.192kHz square-wave enable */
		if (!rtc_read(RTC_CONTROL, &uch, 1)) {
			serial_putstr(FV(msg_no_rtc));
			break;
		}
		if (ISSET(uch, RTC_C_INTCN)) {
			/* Enable */
			uch &= ~(RTC_C_INTCN | RTC_C_RS1 | RTC_C_RS2);
			uch |= CT_8KHZ_CLK;
		} else {
			/* Disable */
			uch |= RTC_C_INTCN;
		}
		(void)rtc_write(RTC_CONTROL, &uch, 1);
		if (!rtc_read(RTC_CONTROL, &uch, 1)) {
			serial_putstr(FV(msg_no_rtc));
			break;
		}
		SERIAL_PRONE(!ISSET(uch, RTC_C_INTCN), "enabled", "disabled");
		SERIAL_PUTSTR(" 8.192kHz square-wave\n");
		break;

	case 'T':
		/* Print the temperature */
		if (!rtc_querytemp(&temp)) {
			serial_putstr(FV(msg_no_rtc));
			return;
		}
		PRINTF("0x%02X 0x%02X\n", temp.buf[0], temp.buf[1]);
		PRINTF("%d.%02d C\n", temp.units, temp.hundreths);

		/* Convert to Fahrenheit avoiding floating point */
		v = temp.units;
		v *= 100;
		v += temp.hundreths;
		v = ((v * 9) / 5) + (32 * 100);
		units = v / 100;
		if (v < 0)
			v = -v;
		hundreths = v % 100;
		PRINTF("%d.%02d F\n", units, hundreths);
		break;

	default:
		/* help */
		SERIAL_PUTSTR(
		    "'t'\t\tdisplay the time\n"
		    "'t H:M:S'\tset the time\n"
		    "'t Y-M-D'\tset the date\n"
		    "'t +N'\t\tadd seconds\n"
		    "'t -N'\t\tsubtract seconds\n"
		    "'tD'\t\tdump all registers\n"
#ifdef HAVE_EEPROM_RTC
		    "'tI%d'\t\tinit aging offset at boot\n"
#endif
		    "'tO%d'\t\taging offset\n"
		    "'tS%d'\t\ttoggle 8.192kHz square wave\n"
		    "'tT'\t\tdisplay the temperature\n"
		    );
		break;
	}
}

/* Force a temperature conversion, return 1 if successful */
static boolean
rtc_convert(void)
{
	uint8_t uch;

	/* Wait until idle */
	if (!rtc_idle())
		return (0);

	/* Read the control register */
	if (!rtc_read(RTC_CONTROL, &uch, 1))
		return (0);

	/* Force a temperature conversion */
	uch |= RTC_C_CONV;
	if (!rtc_write(RTC_CONTROL, &uch, 1))
		return (0);
	return (1);
}

#ifdef SdFat_h
/* SdFat callback */
void
rtc_datetime(uint16_t *datep, uint16_t *timep)
{
	uint8_t h, m, s;
	long delta;
	boolean doquery;
	struct rtc_time *rt;
	static u_long lastmsec = 0;

	/* Try to not hit the clock more than once a minute */
	doquery = 0;
	rt = &rtc_time;

	/* First time through */
	if (lastmsec == 0)
		doquery = 1;

	/* Missing date */
	if (!doquery && rt->year == 0xff)
		doquery = 1;

	/* Avoid wrapping to the next day (last minute of the day) */
	if (!doquery && RTC2HOUR(rt) == 23 && RTC2MIN(rt) >= 59)
		doquery = 1;

	/* More than 60 seconds since our last query */
	if (!doquery) {
		delta = MILLIS_SUB(msec, lastmsec);
		if (delta >= 60L * 1000L)
			doquery = 1;
	}

	/* Only return the date and time if we got something from the clock */
	if (doquery) {
		if (!rtc_query())
			return;
		/* Reset lastmsec */
		lastmsec = msec;
		delta = 0;
	}

	h = RTC2HOUR(rt);
	m = RTC2MIN(rt);
	s = RTC2SEC(rt);

	if (!doquery) {
		delta /= 1000;
		s += delta;
		if (s >= 60) {
			m += s / 60;
			s %= 60;
			if (m >= 60) {
				h += m / 60;
				m %= 60;
			}
		}
	}

	*datep = FAT_DATE(RTC2YEAR(rt), RTC2MONTH(rt), RTC2DAY(rt));
	*timep = FAT_TIME(h, m, s);
}
#endif

/* Return 1 when idle, 0 if there was an I2C error */
static boolean
rtc_idle(void)
{
	int16_t i;
	uint8_t uch;

	/* Wait at least 200ms (temperature conversion time) */
	for (i = 250; i > 0; --i) {
		if (!rtc_read(RTC_STATUS, &uch, 1))
			return (0);
		if ((uch & RTC_S_BSY) == 0)
			return (1);
		delay(1);
	}
	return (0);
}

void
rtc_init(int8_t addr)
{
	uint8_t uch;

	Wire.begin(addr);

	/* Disable interrupts */
	uch = RTC_C_INTCN;
	if (!rtc_write(RTC_CONTROL, &uch, 1)) {
		serial_putstr(FV(msg_no_rtc));
		return;
	}
}

boolean
rtc_pr(boolean query)
{
	struct rtc_time *rt;

	rt = &rtc_time;
	if (query) {
		memset(rt, 0, sizeof(*rt));
		if (!rtc_query())
			return (0);
	}
	PRINTF("%04d-%02d-%02d %02d:%02d:%02d ",
	    RTC2YEAR(rt),
	    RTC2MONTH(rt),
	    RTC2DAY(rt),
	    RTC2HOUR(rt),
	    RTC2MIN(rt),
	    RTC2SEC(rt));
	return (1);
}

/* Return 1 if we were able to get the time */
boolean
rtc_query(void)
{
	struct rtc_time *rt, *rt2, rtc_time2;

	/* Read the time */
	rt2 = &rtc_time2;
	if (!rtc_read(RTC_SECS, (uint8_t *)rt2, sizeof(*rt2)))
		return (0);
	rt = &rtc_time;
	do {
		/* Keep the last ones read */
		memmove(rt, rt2, sizeof(*rt2));
		if (!rtc_read(RTC_SECS, (uint8_t *)rt2, sizeof(*rt2)))
			return (0);
	} while (rt->year != rt2->year ||
	    rt->month != rt2->month ||
	    rt->day != rt2->day ||
	    rt->hour != rt2->hour);

	return (1);
}

/* Return 1 if we were able to get the temperature */
boolean
rtc_querytemp(struct rtc_temp *rtp)
{
	/* Force a temperature conversion */
	if (!rtc_convert())
		return (0);

	/*
	 * "A user-initiated temperature conversion does not affect
	 * the BSY bit for approximately 2ms."
	 */
	delay(3);

	/* Wait for the conversion to complete */
	if (!rtc_idle())
		return (0);

	/* Read temperature */
	if (!rtc_read(RTC_TEMPMSB, rtp->buf, sizeof(rtp->buf)))
		return (0);

	rtp->units = (rtp->buf[0] & RTC_TEMP_MASK);
	if ((rtp->buf[0] & RTC_TEMP_SIGN) != 0)
		rtp->units = -rtp->units;
	rtp->hundreths = (rtp->buf[1] >> 6) * 25;
	return (1);
}

/* Read multiple registers (takes about 565us) */
static boolean
rtc_read(uint8_t reg, uint8_t *up, int8_t size)
{
	boolean ret;

	ret = rtc_read2(reg, up, size);
	if (ret == 0)
		ret = rtc_read2(reg, up, size);
	return (ret);
}

/* Read multiple registers */
static boolean
rtc_read2(uint8_t reg, uint8_t *up, int8_t size)
{
	uint8_t cc;

	Wire.beginTransmission(TWI_RTC);
	Wire.write(reg);
	Wire.endTransmission();

	cc = Wire.requestFrom(TWI_RTC, size);
	if (cc != size)
		return (0);

	while (Wire.available() && size-- > 0)
		*up++ = Wire.read();
	return (1);
}


/* Set the time */
boolean
rtc_set(uint8_t hour, uint8_t min, uint8_t sec)
{
	uint8_t buf[3];

	buf[0] = DEC2BCD(sec);
	buf[1] = DEC2BCD(min);
	buf[2] = DEC2BCD(hour);
	return (rtc_write(RTC_SECS, buf, (int8_t)sizeof(buf)));
}

/* Set the date */
boolean
rtc_setdate(uint16_t year, uint8_t month, uint8_t day)
{
	uint8_t tyear, century;
	uint8_t buf[3];

	if (year < 2000) {
		tyear = year - 1900;
		century = 0;
	} else {
		tyear = year - 2000;
		century = RTC_CENTURY_BIT;
	}
	buf[0] = DEC2BCD(day);
	buf[1] = DEC2BCD(month) | century;
	buf[2] = DEC2BCD(tyear);
	return (rtc_write(RTC_DAY, buf, (int8_t)sizeof(buf)));
}

/* Update the time in certain cases */
boolean
rtc_update(uint8_t hour, uint8_t min, uint8_t sec)
{
	long delta;
	struct rtc_time *rt;

	/* Query RTC */
	if (!rtc_query())
		return (0);

	/* Avoid problems near midnight when we can be really far off */
	if ((hour == 0 && min <= 10) || (hour == 23 && min >= 50)) {
		PRINTF(
		    "rtc_update: ignoring time near midnight: %02u:%02u:%02u\n",
		    hour, min, sec);
		return(1);
	}

	/* Calculate delta to see if we need to do anything */
	rt = &rtc_time;
	delta = (sec + (60L * (min + (60L * hour)))) -
	    (RTC2SEC(rt) + (60L * (RTC2MIN(rt) + (60L * RTC2HOUR(rt)))));
	if (delta < 0)
		delta = -delta;

	/* Only update if we a little bit off */
	if (delta > 2 && delta < 18 * 3600L) {
		if (!rtc_set(hour, min, sec))
			return (0);
		SERIAL_PUTSTR("RTC updated: ");
		PRINTF("(delta %ld) ", delta);
		(void)rtc_pr(0);
		SERIAL_PUTSTR("-> ");
		(void)rtc_pr(1);
		serial_nl();
	}
	return (1);
}

/* Write one or more registers, return 1 if successful */
static boolean
rtc_write(uint8_t reg, uint8_t *up, int8_t size)
{
	Wire.beginTransmission(TWI_RTC);
	Wire.write(reg);
	while (size-- > 0)
		if (!Wire.write(*up++))
			return (0);
	Wire.endTransmission();
	return (1);
}
