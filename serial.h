/* @(#) $Id: serial.h 156 2024-10-19 23:26:34Z leres $ (XSE) */

#ifndef _serial_h_
#define _serial_h_
#include <stdarg.h>
#include <stdio.h>

#define FV(s) ((const __FlashStringHelper*)(s))

#define UART0_BAUD 57600

/* Don't print bell for invalid characters until we've been up this long */
#define QUIET_MS 50

#include "NewSerialPort.h"

#define PRINTF(format, ...) printf_P(PSTR(format), ## __VA_ARGS__)
#define SERIAL_PRONE(first, s1, s2) serial_prone(first, F(s1), F(s2))
#define SERIAL_PUTSTR(s) serial_putstr(F(s))

extern void serial_flush(void);
extern boolean serial_getln(char *, size_t);
extern void serial_init(uint32_t);
extern void serial_nl(void);
extern void serial_poll(void);
extern void serial_prone(boolean, const __FlashStringHelper *,
    const __FlashStringHelper *);
extern int serial_putc(char, FILE *);
extern int serial_putchar(char);
extern void serial_putstr(char *);
extern void serial_putstr(const __FlashStringHelper *);
extern void serial_prspeeds(void);
extern boolean serial_ready(void);
extern boolean serial_speed(uint32_t);
#endif
