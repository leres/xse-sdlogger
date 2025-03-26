/* @(#) $Id: sdlogger.cpp 160 2025-03-24 23:32:11Z leres $ (XSE) */

#if __has_include("local.h")
#include "local.h"
#endif

#include <NewSerialPort.h>

#include <avr/pgmspace.h>
#ifdef notdef
#include <avr/power.h>
#include <avr/sleep.h>
#endif
#include <avr/wdt.h>

#include "sdlogger.h"

#include "cmd.h"
#include "eeprom.h"
#include "led.h"
#include "rtc.h"
#include "sdlogger.h"
#include "serial.h"
#include "status.h"
#include "strings.h"
#include "util.h"

NewSerialPort<0, UART0_SIZE, 0> NewSerial;

/* Blinking LED error codes */
#define ERROR_SD_INIT	LED_MODE_ERR3
#define ERROR_SD_WRITE	LED_MODE_ERR4
#define ERROR_SD_OPEN	LED_MODE_ERR5

/* Globals */
uint8_t debug;
SdFile curdir;
SdVolume volume;
Sd2Card card;
SdFile file;
u_long msec;
u_long card_present_lastms;

/* Forwards */
void append_file(char *);
void blink_error(uint8_t);
void error(const char *);
void datelog(void);
void loop(void);
void newlog(void);
void seqlog(void);
void setup(void);

void
error(const char *str)
{
	PRINTF("error: %s\n", str);
	if (card.errorCode())
		PRINTF("SD error: 0x%x 0x%x\n",
		    card.errorCode(), card.errorData());

	led_mode(ERROR_SD_OPEN);
	for (;;)
		loop();
}

void
setup(void)
{
	char buf[32];

	/* Grab and then zero the MCU status register */
	boot_mcusr = MCUSR;
	MCUSR = 0;

	/* Make sure watchdog is off */
	wdt_disable();

	/* Setup UART1 (configuration/debugging) */
	serial_init(UART1_BAUD);

	/* Initial time */
	msec = millis();

	/* Read eeprom, set defaults */
	eeprom_init();

	/* Setup UART0 */
	NewSerial.begin(eeprom.speed);

	/* SD card detect (internal pullup) */
	pinMode(PIN_SD_CD, INPUT);
	digitalWrite(PIN_SD_CD, HIGH);
	status_set(CARD_PRESENT(), STATUS_STATE_PRESENT);
	card_present_lastms = msec;

	/* Startup mesages */
	serial_nl();
	hello();

        /* Decode MCU status register */
        if (boot_mcusr != 0) {
                SERIAL_PUTSTR("reset:");
		prmcusr(buf, sizeof(buf));
		serial_putstr(buf);
		serial_nl();
        }

	/* Initialize leds */
	led_init();

	/* I2C status */
	status_init();

	/* Real time clock */
	rtc_init(TWI_SDLOGGER);

	/* Register SD date/time callback */
	cmd_init();

	/* Setup SD & FAT */
	if (!card.init(SPI_FULL_SPEED)) {
		SERIAL_PUTSTR("error card.init\n");
		blink_error(ERROR_SD_INIT);
	}
	if (!volume.init(&card)) {
		SERIAL_PUTSTR("error volume.init\n");
		blink_error(ERROR_SD_INIT);
	}
	if (!curdir.openRoot(&volume)) {
		SERIAL_PUTSTR("error openRoot\n");
		blink_error(ERROR_SD_INIT);
	}

	/* First try for date/time file (call rtc_query() twice) */
	if (rtc_query() && rtc_query())
		datelog();

	/* Next try a numbered log */
	newlog();

	/* Finally try a sequential log */
	seqlog();

	/* We should never get this far (all opens failed) */
	led_mode(ERROR_SD_OPEN);
	for (;;)
		loop();
}

void
loop(void)
{
	boolean present;

	/* Time */
	msec = millis();

	/* LED */
	led_poll();

	/* Card detect */
	present = STATUS_PRESENT(status);
	if (present != CARD_PRESENT()) {
		if (MILLIS_SUB(msec, card_present_lastms) >= PRESS_MS) {
			present = !present;
			SERIAL_PUTSTR("card ");
			if (present)
				SERIAL_PUTSTR("inserted");
			else
				SERIAL_PUTSTR("removed");
			serial_nl();
			status_set(present, STATUS_STATE_PRESENT);
			card_present_lastms = msec;
		}
	}

	/* Serial port and commands */
	serial_poll();
	cmd_poll();
}


// Log to a new file everytime the system boots
// Checks the spots in EEPROM for the next available LOG# file name
// Updates EEPROM and then appends to the new log file.
// Limited to 65535 files but this should not always be the case.
void
newlog(void)
{
	char fn[32];

	/* Search for next available log */
	do {
		if (eeprom.logseq == 0xffff - 1) {
			/* Don't set logseq to 0xffff */
			SERIAL_PUTSTR("Too many logs!\n");
			return;
		}

		// Splice the new file number into this file name
		snprintf_P(fn, sizeof(fn), PSTR("LOG%05d.TXT"), eeprom.logseq);

		/* Set the next number number to use */
		++eeprom.logseq;
	} while (!file.open(&curdir, fn, O_CREAT | O_EXCL | O_WRITE));

	if (eeprom_write(0) < 0)
		serial_putstr(FV(msg_eepromfail));

	// Close this new file we just opened
	file.close();

	PRINTF("Created %s\n", fn);
	serial_putstr(FV(msg_prompt));

	append_file(fn);
}

// Log to the same file every time the system boots, sequentially
// Checks to see if the file SEQLOG.txt is available
// If not, create it
// If yes, append to it
// Return 0 on error
// Return anything else on sucess
void
seqlog(void)
{
	char fn[32];

	/* Try to create sequential file */
	strlcpy_P(fn, PSTR("SEQLOG00.TXT"), sizeof(fn));
	if (!file.open(&curdir, fn, O_CREAT | O_WRITE)) {
		PRINTF("error creating %s\n", fn);
		return;
	}
	file.close();

	append_file(fn);
}

// This is the most important function of the device. These loops
// have been tweaked as much as possible.
// Modifying this loop may negatively affect how well the device
// can record at high baud rates.
// Appends a stream of serial data to a given file
// Assumes the currentDirectory variable has been set before entering
// the routine
void
append_file(char *file_name)
{
	size_t cc;
	boolean ok;
	uint16_t idleTime;
	uint8_t localBuffer[32];

	// O_CREAT - create the file if it does not exist
	// O_APPEND - seek to the end of the file prior to each write
	// O_WRITE - open for write
	if (!file.open(&curdir, file_name, O_CREAT | O_APPEND | O_WRITE))
		error("open1");

	/*
	 * This is a trick to make sure first cluster is allocated
	 * Found in Bill's example/beta code
	 */
	if (file.fileSize() == 0) {
		file.rewind();
		file.sync();
	}

	idleTime = 0;

	// Ugly calculation to figure out how many times to loop
	// before we need to force a record (file.sync())
	uint32_t maxLoops;

	// Sync the file every maxLoop # of bytes
	uint16_t timeSinceLastRecord = 0;

	/* Bits per second */
	maxLoops = UART0_BAUD;

	/* Convert to bytes per second */
	maxLoops /= 8;

	/* Convert to # of loops per second */
	maxLoops /= sizeof(localBuffer);
#ifdef notdef
	PRINTF("maxLoops: %d\n", maxLoops);
#endif

	// Start recording incoming characters
	led_red(0);
	status_set(0, STATUS_STATE_ERROR);
	for (;;) {
		/* Give main loop some time */
		loop();

		/* Read from the serial port */
		uint8_t n = NewSerial.read(localBuffer, sizeof(localBuffer));
		if (n > 0) {
			led_red(1);
			cc = file.write(localBuffer, n);
			status_set((cc != n), STATUS_STATE_ERROR);
			/* Hard stop if there were errors */
			if (cc != n)
				blink_error(ERROR_SD_WRITE);

			/* We have characters so reset the idleTime */
			idleTime = 0;

			/* This will force a sync approximately every second */
			if (timeSinceLastRecord++ > maxLoops) {
				timeSinceLastRecord = 0;
				ok = file.sync();
				status_set(!ok, STATUS_STATE_ERROR);
				/* Hard stop if there were errors */
				if (!ok)
					blink_error(ERROR_SD_WRITE);
				led_red(0);
			}
			continue;
		}
		if (idleTime > MAX_IDLE_MS) {
			ok = file.sync();
			status_set(!ok, STATUS_STATE_ERROR);
			/* Hard stop if there were errors */
			if (!ok)
				blink_error(ERROR_SD_WRITE);
			led_red(0);

#ifdef notdef
			// Shut down peripherals we don't need
			power_timer0_disable();
			power_spi_disable();

			// Stop everything and go to sleep. Wake
			// up if serial character received
			sleep_mode();

			// After wake up, power up peripherals
			power_spi_enable();
			power_timer0_enable();
#endif

			// A received character woke us up to reset the idleTime
			idleTime = 0;
			continue;
		}

		// Burn 1ms waiting for new characters coming in
		++idleTime;
		delay(1);
	}

	ok = file.sync();
	status_set(!ok, STATUS_STATE_ERROR);
	/* Hard stop if there were errors */
	if (!ok)
		blink_error(ERROR_SD_WRITE);
	led_red(0);

	// Done recording, close out the file
	ok = file.close();
	status_set(!ok, STATUS_STATE_ERROR);
}

// The following are system functions needed for basic operation
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Blinks the status LEDs to indicate a type of error
void
blink_error(uint8_t e)
{
	PRINTF("blink_error %d\n", e);
	led_mode(e);
	for (;;)
		loop();
}

static const char dottxt[] PROGMEM = ".TXT";

void
datelog(void)
{
	char *cp;
	int8_t i, size, cc;
	uint16_t uv;
	struct rtc_time *rt;
	char fn[sizeof("12345678.TXT")];

	rt =  &rtc_time;
	cp = fn;
	size = sizeof(fn);

	/* 16 bits of y/m/d fits in 4 chars of base 36 */
	uv = FAT_DATE(RTC2YEAR(rt), RTC2MONTH(rt), RTC2DAY(rt));
	cc = ui2str(uv, cp, size, 36);
	if (cc < 0)
		return;

	/* Zero pad to 4 chars */
	i = 4 - cc;
	if (i > 0) {
		memmove(cp + i, cp, cc + 1);
		memset(cp, '0', i);
		cc += i;
	}
	cp += cc;
	size -= cc;

	/* 16 bits of h:m:(s/2) fits in 4 chars of base 36 */
	uv = FAT_TIME(RTC2HOUR(rt), RTC2MIN(rt), RTC2SEC(rt));
	cc = ui2str(uv, cp, size, 36);
	if (cc < 0)
		return;

	/* Zero pad to 4 chars */
	i = 4 - cc;
	if (i > 0) {
		memmove(cp + i, cp, cc + 1);
		memset(cp, '0', i);
		cc += i;
	}
	cp += cc;
	size -= cc;

	/* add ".TXT" */
	if (size < (int8_t)sizeof(dottxt))
		return;
	strlcpy_P(cp, dottxt, size);

	if (!file.open(&curdir, fn, O_CREAT | O_WRITE | O_APPEND))
		return;

	/* Close this new file we just opened */
	file.close();

	PRINTF("Created %s\n", fn);
	serial_putstr(FV(msg_prompt));

	append_file(fn);
}
