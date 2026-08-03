/* @(#) $Id: status.cpp 176 2026-08-02 23:56:51Z leres $ (XSE) */

#if __has_include("local.h")
#include "local.h"
#endif

#include <Wire.h>

#include "sdlogger.h"

#include "serial.h"
#include "status.h"

/* Globals */
uint8_t status;
extern int8_t mywireaddr;

/* Forwards */
static void status_onrequest(void);

void
status_init(void)
{
	int8_t addr;

	addr = TWI_SDLOGGER;
	status |= STATUS_STATE_ERROR;
	if (mywireaddr == 0) {
		Wire.begin(addr);
		mywireaddr = addr;
	} else if (mywireaddr != addr) {
		PRINTF("status_init: invalid addr, aborting (%d != %d)\n",
		    mywireaddr, addr);
		return;
	}
	Wire.onRequest(status_onrequest);
}

static void
status_onrequest(void)
{
	Wire.write(status);
}

void
status_set(boolean on, uint8_t ui)
{
	if (on)
		status |= ui;
	else
		status &= ~ui;
}
