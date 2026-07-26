# @(#) $Id: Makefile 174 2026-07-26 21:38:59Z leres $ (XSE)

TARGET=		sdlogger
ARDUINO_BOARD=	atmega1284
F_CPU=		14745600L

AVRDUDE_PROGRAMMER= usbtiny
AVRDUDE_FLAGS+=-B 1

ARDUINO_MK_DIR=	/usr/local/arduino-bsd-mk

ARDUINO_LIBS=	EEPROM SD SPI Wire
ARDUINO_DIR=	/opt/arduino

MAKEOBJDIRPREFIX=/usr/obj

SRCS=		sdlogger.cpp \
		NewSerialPort.cpp \
		cmd.cpp \
		eeprom.cpp \
		led.cpp \
		rtc.cpp \
		serial.cpp \
		sstrings.cpp \
		status.cpp \
		util.cpp

HFILES=		NewSerialPort.h \
		cmd.h \
		eeprom.h \
		led.h \
		rtc.h \
		sdlogger.h \
		serial.h \
		sstrings.h \
		status.h \
		util.h \
		version.h

COMMON_CFLAGS+= -Wall -Werror -Wextra -Wno-unused-parameter -Wunreachable-code
COMMON_CFLAGS+= -fdiagnostics-color=never
COMMON_CFLAGS+= -DTWI_FREQ=200000L
#COMMON_CFLAGS+= -DUART0_SIZE=2000 -DIBUFSIZE=32
COMMON_CFLAGS+= -DUART0_SIZE=8192 -DIBUFSIZE=256
#COMMON_CFLAGS+= -DDEBUG
#COMMON_CFLAGS+= -DUSE_WDT

ARDUINO_CFLAGS+= $(COMMON_CFLAGS) -Wstrict-prototypes -Wtraditional
ARDUINO_CXXFLAGS+= $(COMMON_CFLAGS)

NO_EXTRADEPEND=

.if exists(${.CURDIR}/.svn)
version.h:
	@${.CURDIR}/version.sh ${.CURDIR}
.endif

# Always update version.h when necessary
.BEGIN:
.if ${.CURDIR} != ${.OBJDIR} && \
    exists(${.CURDIR}/version.h) && exists(${.OBJDIR}/version.h)
	@echo "ERROR: Both version.h and obj/version.h exist" 1>&2
	@false
.endif
.if empty(.TARGETS:Mclean) && empty(.TARGETS:Mcleandepend) && \
    empty(.TARGETS:Mdepend) && empty(.TARGETS:Mobj) && \
    empty(.TARGETS:Mobjlink)
.if exists(${.CURDIR}/.svn)
	@${.CURDIR}/version.sh ${.CURDIR}
.endif
	@cd ${.CURDIR} && ${MAKE} depend
.endif

CLEANFILES+=	.depend
.if exists(${.CURDIR}/.svn)
CLEANFILES+=	version.h
.endif

.if exists(${.CURDIR}/.svn)
TOPVERSION=	${:!cat ${.CURDIR}/VERSION!}
SVNVERSION=	${:!cd ${.CURDIR} && svnversion!}

hex: ${TARGET}.hex
	@test ${SVNVERSION} = ${SVNVERSION:C/[M:]//} || \
	    (echo ERROR: subversion checkout needs updating ; exit 2)
	cp -p ${TARGET}.hex ${.CURDIR}/archive/${TOPVERSION}.${SVNVERSION}.hex
	chmod a-w,a+x ${.CURDIR}/archive/${TOPVERSION}.${SVNVERSION}.hex
	cp -p ${TARGET}.elf ${.CURDIR}/archive/${TOPVERSION}.${SVNVERSION}.elf
	chmod a-w,a+x ${.CURDIR}/archive/${TOPVERSION}.${SVNVERSION}.elf
.endif

force: /tmp

tar: force
	@cd ${.CURDIR} ; sname=xse-$(TARGET) ; \
	    name=$$sname-`cat ${.CURDIR}/VERSION`; \
	    list="" ; tar="tar chf" ; temp="$$name.tar.gz" ; \
	    for i in `cat ${.CURDIR}/FILES` ; \
	    do list="$$list $$name/$$i" ; done; \
	    echo \
	    "rm -f $$name; ln -s . $$name" ; \
	     rm -f $$name; ln -s . $$name ; \
	    echo \
	    "$$tar - [lots of files] | gzip > ${.CURDIR}$$temp" ; \
	     $$tar - $$list | gzip > $$temp ; \
	    echo \
	    "rm -f $$name" ; \
	     rm -f $$name

.include <$(ARDUINO_MK_DIR)/bsd.arduino.mk>
