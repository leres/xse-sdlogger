/* @(#) $Id: util.h 156 2024-10-19 23:26:34Z leres $ (XSE) */

#ifndef _util_h
#define _util_h
#define PRHEX(msg, cp, n) prhex(F(msg), cp, n)

/* https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html */
#define STR(s) _STR(s)
#define _STR(s) #s

/* CPU speed string */
#if F_CPU == 32000000UL
#define MHZ_STR " @ 32 MHz"
#elif F_CPU == 20000000UL
#define MHZ_STR " @ 20 MHz"
#elif F_CPU == 16000000UL
#define MHZ_STR " @ 16 MHz"
#elif F_CPU == 14745600L
#define MHZ_STR " @ 14.7456 MHz"
#elif F_CPU == 8000000UL
#define MHZ_STR " @ 8 MHz"
#else
#define MHZ_STR ""
#endif

extern unsigned int __data_start;
extern unsigned int __data_end;
extern unsigned int __bss_start;
extern unsigned int __bss_end;
extern unsigned int _etext;
extern unsigned int _edata;

/* XXX Used to determine the size of .progmem by examining the linker map */
extern unsigned int __ctors_start;
extern unsigned int __trampolines_end;

extern void check_mem(void);
extern void hello(void);
extern void prhex(const __FlashStringHelper *, const u_char *, int);
extern void prts(void);
extern void prmcusr(char *, int8_t);
extern void reset(void);
extern int8_t ui2str(uint16_t, char *, int8_t, int8_t);

extern uint8_t *heapptr, *stackptr;
extern uint8_t boot_mcusr;
#endif
