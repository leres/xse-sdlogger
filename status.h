/* @(#) $Id: status.h 128 2021-05-16 18:29:46Z leres $ (XSE) */

#ifndef _status_h_
#define _status_h_
#define STATUS_STATE	0

/* "Ok, Houston, we've had a problem here" */
#define STATUS_STATE_ERROR	0x01

/* Unsynced data exists */
#define STATUS_STATE_DIRTY	0x02

/* Card is present */
#define STATUS_STATE_PRESENT	0x04

#define STATUS_ERROR(s)		ISSET((s), STATUS_STATE_ERROR)
#define STATUS_DIRTY(s)		ISSET((s), STATUS_STATE_DIRTY)
#define STATUS_PRESENT(s)	ISSET((s), STATUS_STATE_PRESENT)

extern uint8_t status;

extern void status_init(void);
extern void status_set(boolean, uint8_t);
#endif
