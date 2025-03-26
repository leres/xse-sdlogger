/* @(#) $Id: rtc.h 154 2024-10-09 01:31:59Z leres $ (XSE) */
#ifndef _rtc_h
#define _rtc_h

/* DS3231M status addresses */
#define RTC_SECS	0x00		/* seconds 00-59 */
#define RTC_MINS	0x01		/* minutes 00-59 */
#define RTC_HOURS	0x02		/* hours 1-12 + AM/PM or 00-23 */
#define RTC_DOW		0x03		/* day of week 1-7 */
#define RTC_DAY		0x04		/* day of month 01-31 */
#define RTC_MON		0x05		/* month 01-12 + century */
#define RTC_YEAR	0x06		/* year 00-99 */
#define RTC_A1_SECS	0x07		/* alarm1 seconds 00-59 */
#define RTC_A1_MINS	0x08		/* alarm1 minutes 00-59 */
#define RTC_A1_HOURS	0x09		/* alarm1 hours 1-12 + AM/PM or 00-23 */
#define RTC_A1_DAY	0x0A		/* alarm1 day of week/month 1-7/1-31 */
#define RTC_A2_MINS	0x0B		/* alarm2 minutes 00-59 */
#define RTC_A2_HOURS	0x0C		/* alarm2 hours 1-12 + AM/PM or 00-23 */
#define RTC_A2_DAY	0x0D		/* alarm2 day of week/month 1-7/1-31 */
#define RTC_CONTROL	0x0E		/* control */
#define RTC_STATUS	0x0F		/* status */
#define RTC_OFFSET	0x10		/* aging offset 0x81-0x7F */
#define RTC_TEMPMSB	0x11		/* temperature MSB */
#define RTC_TEMPLSB	0x12		/* temperature LSB */

/* Masks and bits */
#define RTC_MONTH_MASK	0x1F
#define RTC_CENTURY_BIT	0x80
#define RTC_TEMP_MASK	0x7F
#define RTC_TEMP_SIGN	0x80

/* RTC_CONTROL bits */
#define RTC_C_A1IE	(1 << 0)	/* alarm1 interrupt enable */
#define RTC_C_A2IE	(1 << 1)	/* alarm2 interrupt enable */
#define RTC_C_INTCN	(1 << 2)	/* interrupt control */
#define RTC_C_RS1	(1 << 3)	/* square wave rate select 1 */
#define RTC_C_RS2	(1 << 4)	/* square wave rate select 2 */
#define RTC_C_CONV	(1 << 5)	/* convert temperature */
#define RTC_C_BBSQW	(1 << 6)	/* enable battery-backed square-wave */
#define RTC_C_EOSC	(1 << 7)	/* (not) enable oscillator */

/* Square wave rates */
#define CT_1HZ_CLK	(                    0)	/* 1Hz */
#define CT_1KHZ_CLK	(            RTC_C_RS1)	/* 1.024kHz */
#define CT_4KHZ_CLK	(RTC_C_RS2            )	/* 4.096kHz */
#define CT_8KHZ_CLK	(RTC_C_RS2 | RTC_C_RS1)	/* 8.192kHz */

/* RTC_STATUS bits */
#define RTC_S_A1F	(1 << 0)	/* alarm1 flag */
#define RTC_S_A2F	(1 << 1)	/* alarm1 flag */
#define RTC_S_BSY	(1 << 2)	/* busy */
#define RTC_S_EN32KHZ	(1 << 3)	/* enable 32.768kHz output */
#define RTC_S_OSF	(1 << 7)	/* 32.768kHz output */

/* BCD to decimal */
#define BCD2DEC(v)	(((((v) & 0xF0) >> 4) * 10) + ((v) & 0x0F))

/* Decimal to BCD */
#define DEC2BCD(v)	((((v) / 10) << 4) + (((v) % 10) & 0x0F))

/* Two's Complement */
#define TWOSCOMPLEMENT(v) (~(v) + 1)

/* Extract fields */
#define RTC2CENTURY(p) \
    ((int16_t)((((p)->month & RTC_CENTURY_BIT) == 0) ? 1900 : 2000))
#define RTC2YEAR(p) ((int16_t)BCD2DEC((p)->year) + RTC2CENTURY(p))
#define RTC2MONTH(p) BCD2DEC((p)->month & RTC_MONTH_MASK)
#define RTC2DAY(p) BCD2DEC((p)->day)
#define RTC2HOUR(p) BCD2DEC((p)->hour)
#define RTC2MIN(p) BCD2DEC((p)->min)
#define RTC2SEC(p) BCD2DEC((p)->sec)

struct rtc_time {
	uint8_t		sec;		/* seconds */
	uint8_t		min;		/* minutes */
	uint8_t		hour;		/* hours */
	uint8_t		dow;		/* day of week */
	uint8_t		day;		/* day */
	uint8_t		month;		/* month */
	uint8_t		year;		/* year */
};

extern struct rtc_time rtc_time;

/* Celsius */
struct rtc_temp {
	uint8_t units;
	uint8_t hundreths;
	u_char buf[2];
};

#ifdef RTC_AGING
extern void rtc_aging(int8_t);
#endif
extern void rtc_cmd(char *);
#ifdef SdFat_h
extern void rtc_datetime(uint16_t *, uint16_t *);
#endif
extern void rtc_init(int8_t);
extern boolean rtc_pr(boolean);
extern boolean rtc_query(void);
extern boolean rtc_querytemp(struct rtc_temp *);
extern boolean rtc_set(uint8_t, uint8_t, uint8_t);
extern boolean rtc_setdate(uint16_t, uint8_t, uint8_t);
extern boolean rtc_update(uint8_t, uint8_t, uint8_t);
#endif
