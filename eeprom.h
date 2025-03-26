/* @(#) $Id: eeprom.h 156 2024-10-19 23:26:34Z leres $ (XSE) */

#ifndef _eeprom_h
#define _eeprom_h
struct eeprom {
	uint16_t logseq;
	uint8_t debug;
	uint32_t speed;
};

/* eeprom index to store eeprom struct*/
#define EEPROM_START	32

extern struct eeprom eeprom;

extern void eeprom_cmd(char *);
extern void eeprom_init(void);
extern void eeprom_read(void);
extern void eeprom_report(void);
extern int8_t eeprom_write(boolean);
#endif
