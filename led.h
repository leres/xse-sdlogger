/* @(#) $Id: led.h 52 2017-02-21 01:21:29Z leres $ (XSE) */

#ifndef _led_h_
#define _led_h_
#define LED_MODE_RUN	1
#define LED_MODE_ERR2	2
#define LED_MODE_ERR3	3
#define LED_MODE_ERR4	4
#define LED_MODE_ERR5	5

extern void led_green(boolean);
extern void led_init(void);
extern void led_mode(int8_t);
extern void led_poll(void);
extern void led_red(boolean);
#endif
