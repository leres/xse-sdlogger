# XSE SDlogger

This is my re-implementation of the [seeed studio](https://www.seeedstudio.com/) SDLogger serial TTL logger (which itself is a seeed studio SDLogger of the  [SparkFun](https://www.sparkfun.com/) Openlog logger).

## Firmware Changes

Here are some of the changes I made to the firmware:

 - Used the second serial port to implement a configuration/debugging console

 - Stored the log sequence number in eeprom

 - Stored the capture UART speed in eeprom (the configuration/debugging console UART is nailed to 57600 baud)

 - Detect when the SD card has been inserted or removed

 - The blue LED blinks one or more times per second:

      | Number of Blinks  |Indication               |
      |   :---:   | ---                             |
      |       1 	| Normal operation                |
      |       3 	| SD init error                   |
      |       4 	| SD write error                  |
      |       5 	| open error (all methods failed) |

 - The red LED indicates unflushed data is present.

 - Added I2C code that allows clients to determine if the SD card is present, if there is unflushed data, or if there is an error condition.

 - Used 8K (half) of the 16K of available SRAM for the receive buffer. OpenLog uses 512 and SDLogger uses 2000 bytes.

 - Added support for reading a Maxim DS3231 real-time clock chip via I2C. This allows file system timestamped log files and also encoding the date and time in the DOS 8.3 filename. By using the characters A-Z and 0-9 it's possible to encode 16 bits into 4 characters of base 36. So year, month, and day are stored in the first 4 characters and hours, minutes, and seconds are stored in the last 4 characters. For example, 0H0Z0W86.TXT decodes to January 19, 2023 at 7:25:26 pm (local time zone). Note that the FAT file system only allows for even seconds of resolution; there is literally no room to store the odd bit.
  
 - Added a script ([Renameclass2applog](https://raw.githubusercontent.com/leres/xse-sdlogger/refs/heads/main/scripts/Renameclass2applog?token=GHSAT0AAAAAAC3Y6XTUA3XQTTPEEFYEMJDEZ7ELW4Q)) to rename 8.3 files to a human readable format.

## Building

[FreeBSD](https://www.freebsd.org/) is my primary development environment. I prefer building Arduino sketches from the command line. Makefile uses [bsd.arduino.mk](https://xse.com/leres/software/arduino/arduino-bsd-mk.html) which is provided by the [devel/arduino-bsd-mk](https://svnweb.freebsd.org/ports/head/devel/arduino-bsd-mk/) port.

To build you will need to have these packages installed:

 - arduino
 - arduino-bsd-mk
 - avr-binutils
 - avr-gcc
 - avr-libc
 - avrdude

Then it's just a matter of typing "make".
