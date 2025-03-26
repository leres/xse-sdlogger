/* @(#) $Id: status.cpp 148 2024-04-28 17:09:24Z leres $ (XSE) */

#if __has_include("local.h")
#include "local.h"
#endif

#include <Wire.h>

#include "sdlogger.h"

#include "status.h"

/* Globals */
uint8_t status;

/* Locals */
static uint8_t reg;

/* Forwards */
static void status_onreceive(int);
static void status_onrequest(void);

void
status_init(void)
{
	status |= STATUS_STATE_ERROR;
	Wire.begin(TWI_SDLOGGER);
	Wire.onReceive(status_onreceive);
	Wire.onRequest(status_onrequest);
}

static void
status_onreceive(int n)
{
	/* Update register number */
	while (n-- > 0)
		reg = Wire.read();
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
