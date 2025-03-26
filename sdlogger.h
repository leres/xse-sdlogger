/* @(#) $Id: sdlogger.h 156 2024-10-19 23:26:34Z leres $ (XSE) */

#ifndef _sdlogger_h_
#define _sdlogger_h_

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <Arduino.h>

#include <SdFat.h>

#define ISSET(v, bm) (((v) & (bm)) == (bm))

/* Subtract t2 from t1; handles overflow correctly */
#define MICROS_SUB(t1, t2) \
    (((t1) >= (t2)) ? (t1) - (t2) : (t1) + (UINT32_MAX - (t2)))
#define MILLIS_SUB(t1, t2) MICROS_SUB((t1), (t2))

/* Default when eeprom.speed is bogus */
#ifndef UART0_BAUD
#define UART0_BAUD 57600
#endif

#ifndef UART0_SIZE
#define UART0_SIZE 2000
#endif

#ifndef UART1_BAUD
#define UART1_BAUD 57600
#endif

/* ms to wait before going to sleep */
#ifndef MAX_IDLE_MS
#define MAX_IDLE_MS	500
#endif

#if defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
#define LED_GREEN	0		/* d0 */
#define LED_RED		1		/* d1 */

					/* d4: SPI/SS */
					/* d5: SPI/MOSI */
					/* d6: SPI/MISO */
					/* d7: SPI/SCK */

/* Serial: command and debugging console */
					/* d8: uRX1 */
					/* d9: used for TX0 */
/* Serial: logger input */
					/* d10: RX2 */
					/* d11: TX2 */

/* I2C/TWI */
					/* d16: SCL */
					/* d17: SDA */

#define PIN_SD_CD	18		/* d18: card detect (CD) */
#define PIN_SD_WP	19		/* d19: write protect (not used) */

					/* a3: (d28) */
					/* a2: (d29) */
					/* a1: (d30) */
					/* a0: (d31) */
#endif

#ifndef LED_GREEN
#error "PINs not defined for this AVR chip type"
#endif

#define TWI_SDLOGGER	32
#define TWI_RTC		0x68		/* DS3231 */

#define CARD_PRESENT()	(!digitalRead(PIN_SD_CD))

#define PRESS_MS	33		/* debounce duration */

/* XXX */
typedef unsigned long u_long;
typedef unsigned char u_char;

extern uint8_t boot_mcusr;
extern uint8_t debug;

extern SdFile curdir;
extern SdVolume volume;
extern Sd2Card card;
extern SdFile file;
extern u_long msec;
#endif
