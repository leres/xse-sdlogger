/* @(#) $Id: cmd.cpp 156 2024-10-19 23:26:34Z leres $ (XSE) */

#if __has_include("local.h")
#include "local.h"
#endif

#include <ctype.h>
#include <string.h>

#include <Wire.h>

#include "sdlogger.h"

#include "cmd.h"
#include "eeprom.h"
#include "rtc.h"
#include "serial.h"
#include "status.h"
#include "strings.h"
#include "util.h"
#include "version.h"

/* Locals */

/* Forwards */
static void cmd_cmd(char *);
static void ls(SdFile, uint8_t flags = 0, uint8_t indent = 0);
static void printDirName(const dir_t&, uint8_t);
static void printFatDate(uint16_t);
static void printFatTime(uint16_t);
static void showdisk(void);

static void
cmd_cmd(char *s)
{
	u_long uv;
	int16_t v;
	uint16_t u16;
	boolean sawcr, sawnl;
	char ch, *p, *ep;
	SdFile tfile;

	if (*s == '\0')
		goto done;

	if (strncmp_P(s, PSTR("cat"), 3) == 0) {
		s += 3;
		if (!isblank(*s))
			goto help;
		++s;
		while (isblank(*s))
			++s;
		if (*s == '\0')
			goto help;
		if (!tfile.open(&curdir, s, O_READ)) {
			PRINTF("Can't open %s\n", s);
			goto done;

		}
		if (tfile.isDir() || tfile.isSubDir()) {
			SERIAL_PUTSTR("Can't cat directory\n");
			tfile.close();
			goto done;
		}

		/* Cat the file displaying unprintable */
		sawcr = 0;
		sawnl = 1;
		while ((v = tfile.read()) != -1) {
			ch = v;
			sawnl = 0;
			if (ch == '\r') {
				sawcr = 1;
				continue;
			}
			if (ch == '\n') {
				serial_nl();
				sawnl = 1;
				sawcr = 0;
				continue;
			}
			if (sawcr)
				serial_nl();
			sawcr = 0;
			if (isblank(ch)) {
				serial_putchar(ch);
				continue;
			}
			if (!isprint(ch)) {
				PRINTF("\\%03o", ch);
				continue;
			}
			serial_putchar(ch);
		}
		tfile.close();
		if (!sawnl)
			serial_nl();

		goto done;
	}

	if (strcmp_P(s, PSTR("ls")) == 0) {
		PRINTF("Volume is FAT %d\n", volume.fatType());
		ls(curdir, LS_DATE | LS_SIZE | LS_R);
		goto done;
	}

	if (strncmp_P(s, PSTR("rm"), 2) == 0) {
		s += 2;
		if (!isblank(*s))
			goto help;
		++s;
		while (isblank(*s))
			++s;
		if (*s == '\0')
			goto help;
		if (!tfile.open(&curdir, s, O_WRITE)) {
			PRINTF("Can't open %s\n", s);
			goto done;

		}
		if (tfile.isDir() || tfile.isSubDir()) {
			SERIAL_PUTSTR("Can't remove directory\n");
			tfile.close();
			goto done;
		}
		if (!tfile.remove()) {
			PRINTF("rm %s failed\n", s);
			tfile.close();
		}
		tfile.close();

		goto done;
	}

	if (strcmp_P(s, PSTR("zero")) == 0) {
		/* Zero the newseq counter */
		eeprom.logseq = 0;
		(void)eeprom_write(1);
		goto done;
	}

	if (strcmp_P(s, PSTR("sync")) == 0) {
		if (!file.sync())
			SERIAL_PUTSTR("file.sync() failed\n");
		if (!curdir.sync())
			SERIAL_PUTSTR("file.sync() failed\n");
		goto done;
	}

	p = s + 1;
	while (*p == ' ')
		++p;

	switch (*s) {

	case 'd':
		/* debug level */
		if (*p != '\0') {
			uv = strtoul(p, &ep, 10);
			if (*ep != '\0' || uv >= 255) {
				serial_putstr(FV(msg_badvalue));
				break;
			}
			debug = uv;
			eeprom.debug = debug;
			(void)eeprom_write(0);
		}
		PRINTF("debug: %d\n", debug);
		break;

	case 'e':
		/* EEPROM debugging */
		eeprom_cmd(p);
		break;

	case 'r':
		/* TEXT */
		PRINTF("text size: %5u\n", ARDUINO_MAXIMUM_SIZE);
		u16 = (uint16_t)(&_etext) + (((uint16_t)(&_edata)) - 256);
		PRINTF("text used: %5u\n", u16);
		PRINTF("text left: %5u\n", ARDUINO_MAXIMUM_SIZE - u16);
		PRINTF("(progmem): %5u\n",
		    (int)&__ctors_start - (int)&__trampolines_end);
		/* RAM */
		check_mem();
		PRINTF(" ram size: %5d\n", (RAMEND - (int)&__data_start) + 1);
		PRINTF(" bss size: %5d\n", (int)&__bss_end - (int)&__bss_start);
		PRINTF("data size: %5d\n",
		    (int)&__data_end - (int)&__data_start);
		PRINTF(" ram left: %5u\n", stackptr - heapptr);
		break;

	case 'R':
		SERIAL_PUTSTR("Goodbye!\n");
		serial_flush();
		reset();
		break;

	case 's':
		/* Show things */
		PRINTF("logseq: %d\n", eeprom.logseq);
		SERIAL_PUTSTR("status:");
		if ((status &  STATUS_STATE_ERROR) != 0)
			SERIAL_PUTSTR(" ERROR");
		if ((status &  STATUS_STATE_DIRTY) != 0)
			SERIAL_PUTSTR(" DIRTY");
		serial_nl();
		showdisk();
		break;

	case 'S':
		/* Card status */
		SERIAL_PUTSTR("logger status is ");
		SERIAL_PRONE(STATUS_ERROR(status), "error", "ok");
		serial_nl();
		SERIAL_PUTSTR("filesystem is ");
		SERIAL_PRONE(STATUS_DIRTY(status), "dirty", "clean");
		serial_nl();
		SERIAL_PUTSTR("card is ");
		SERIAL_PRONE(STATUS_PRESENT(status), "present", "not present");
		serial_nl();
		break;

	case 't':
		/* RTC */
		rtc_cmd(p);
		break;

	case 'T':
		/* Time since boot */
		prts();
		serial_nl();
		break;

	case 'V':
		/* version */
		hello();
		break;

	case '?':
	case 'h':
		/* help */
		SERIAL_PUTSTR(
		    "\"cat\"\tdisplay a file\n"
		    "\"ls\"\tlist files\n"
		    "\"rm\"\tremove a file\n"
		    "\"sync\"\tsync file and directory\n"
		    "\"zero\"\tzero newseq\n"
		    "'d'\tdebug level\n"
		    "'e'\teeprom cmd\n"
		    "'r'\tRAM left\n"
		    "'R'\tsoft reset\n"
		    "'s'\tshow\n"
		    "'S'\tcard status\n"
		    "'t'\tRTC\n"
		    "'T'\ttime since boot\n"
		    "'V'\tversion\n"
		    "'h'\thelp\n");
		break;

	case '#':
		/* ignore */
		break;

	default:
help:
		serial_putstr(FV(msg_errmsg));
		break;
	}
done:
	serial_putstr(FV(msg_prompt));
}

void
cmd_init(void)
{
	SdFile::dateTimeCallback(rtc_datetime);
}

void
cmd_poll(void)
{
	char buf[64];

	if (serial_getln(buf, sizeof(buf)))
		cmd_cmd(buf);
}

/* SdFile::printDirName() that uses NewSerial */
static void
printDirName(const dir_t& dir, uint8_t width)
{
	uint8_t w = 0;

	for (uint8_t i = 0; i < 11; i++) {
		if (dir.name[i] == ' ')
			continue;
		if (i == 8) {
			serial_putchar('.');
			++w;
		}
		serial_putchar(dir.name[i]);
		++w;
	}
	if (DIR_IS_SUBDIR(&dir)) {
		serial_putchar('/');
		++w;
	}
	while (w < width) {
		serial_putchar(' ');
		++w;
	}
}

static void
printFatDate(uint16_t fatDate)
{
	PRINTF("%04d-%02d-%02d",
	    FAT_YEAR(fatDate), FAT_MONTH(fatDate), FAT_DAY(fatDate));
}

static void
printFatTime(uint16_t fatTime)
{
	PRINTF("%02d:%02d:%02d",
	    FAT_HOUR(fatTime), FAT_MINUTE(fatTime), FAT_SECOND(fatTime));
}

/* SdFile::ls() that uses NewSerial */
static void
ls(SdFile dp, uint8_t flags, uint8_t indent)
{
	dir_t d, *p;

	p = &d;
	dp.rewind();
	while (dp.readDir(*p) > 0) {
		/* done if past last used entry */
		if (p->name[0] == DIR_NAME_FREE)
			break;

		/* skip deleted */
		if (p->name[0] == DIR_NAME_DELETED)
			continue;

		/* skip dot files */
		if (p->name[0] == '.')
			continue;

		/* skip if directory */
		if (!DIR_IS_FILE_OR_SUBDIR(p))
			continue;

		/* Pirnt the name */
		printDirName(*p, (flags & (LS_DATE | LS_SIZE)) ? 14 : 0);

		/* Print date/time */
		if (flags & LS_DATE) {
			printFatDate(p->lastWriteDate);
			serial_putchar(' ');
			printFatTime(p->lastWriteTime);
		}

		/* print size if requested */
		if (!DIR_IS_SUBDIR(p) && (flags & LS_SIZE) != 0)
			PRINTF(" %lu", p->fileSize);
		serial_nl();

		/* list subdirectory content if requested */
		if ((flags & LS_R) && DIR_IS_SUBDIR(p)) {
			uint16_t index = (dp.curPosition() / 32) - 1;
			SdFile s;
			if (s.open(dp, index, O_READ))
				ls(s, flags, indent + 2);
			dp.seekSet(32 * (index + 1));
		}
	}
}

static void
showdisk(void)
{
	cid_t cid;
	csd_t csd;
	uint32_t cardSize;
	uint8_t cardt;
	int16_t year;

	SERIAL_PUTSTR("card type: ");
	cardt = card.type();
	switch (cardt) {

	case SD_CARD_TYPE_SD1:
		SERIAL_PUTSTR("SD1");
		break;

	case SD_CARD_TYPE_SD2:
		SERIAL_PUTSTR("SD2");
		break;

	case SD_CARD_TYPE_SDHC:
		SERIAL_PUTSTR("SDHC");
		break;

	default:
		PRINTF("%d", cardt);
		break;
	}
	serial_nl();

	if (!card.readCID(&cid)) {
		SERIAL_PUTSTR("readCID failed\n");
		return;
	}

	PRINTF("manufacturer id: %x\n", cid.mid);
	PRINTF("OEM id: %c%c\n", cid.oid[0], cid.oid[1]);
	PRINTF("product: %c%c%c%c%c\n",
	    cid.pnm[0], cid.pnm[1], cid.pnm[2], cid.pnm[3], cid.pnm[4]);
	PRINTF("version: %d.%d\n", cid.prv_n, cid.prv_m);
	PRINTF("serial number: %lu\n", cid.psn);
	year = 2000 + (cid.mdt_year_high << 4) + cid.mdt_year_low;
	PRINTF("Manufacturing date: %d/%d\n", cid.mdt_month, year);

	cardSize = card.cardSize();
	if (cardSize == 0 || !card.readCSD(&csd)) {
		SERIAL_PUTSTR("readCSD failed\n");
		return;
	}

	/* 512 blocks -> kbytes */
	PRINTF("card size: %lu KB\n", cardSize / 2);
}
