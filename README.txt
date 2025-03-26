This is the original first comment block from OpenLog_v3.ino

/*
 12-3-09
 Nathan Seidle
 SparkFun Electronics 2012

 OpenLog hardware and firmware are released under the Creative
 Commons Share Alike v3.0 license.
 http://creativecommons.org/licenses/by-sa/3.0/
 Feel free to use, distribute, and sell varients of OpenLog. All
 we ask is that you include attribution of 'Based on OpenLog by
 SparkFun'.

 OpenLog is based on the work of Bill Greiman and sdfatlib:
 http://code.google.com/p/sdfatlib/

 OpenLog is a simple serial logger based on the ATmega328 running
 at 16MHz. The ATmega328 is able to talk to high capacity (larger
 than 2GB) SD cards. The whole purpose of this logger was to create
 a logger that just powered up and worked. OpenLog ships with an
 Arduino/Optiboot 115200bps serial bootloader running at 16MHz so
 you can load new firmware with a simple serial connection.

 OpenLog automatically works with 512MB, 1GB, 2GB, 4GB, 8GB, and
 16GB microSD cards. We recommend FAT16 for 2GB and smaller cards.
 We recommend FAT32 for 4GB and larger cards.

 OpenLog runs at 9600bps by default. This is configurable to 2400,
 4800, 9600, 19200, 38400, 57600, and 115200bps. You can alter all
 settings including baud rate and escape characters by editing
 config.txt found on OpenLog.

 Type '?' to get a list of supported commands.

 During power up, you will see '12<'. '1' indicates the serial
 connection is established. '2' indicates the SD card has been
 successfully initialized. '<' indicates OpenLog is ready to receive
 serial characters.

 Recording constant 115200bps datastreams are supported. Throw it
 everything you've got! To acheive this maximum record rate, please
 use the SD card formatter from :
 http://www.sdcard.org/consumers/formatter/. The fewer files on the
 card, the faster OpenLog is able to begin logging.  200 files is
 ok. 2GB worth of music and pictures is not.

 To a lower dir, use 'cd ..' instead of 'cd..'.

 Only standard 8.3 file names are supported. "12345678.123" is
 acceptable. "123456789.123" is not.

 All file names are pushed to upper case. "NewLog.txt" will become
 "NEWLOG.TXT".

 Type 'set' to enter baud rate configuration menu. Select the baud
 rate and press enter. You will then see a message 'Going to
 9600bps...' or some such message. You will need to power down
 OpenLog, change your system UART settings to match the new OpenLog
 baud rate and then power OpenLog back up.

 If you get OpenLog stuck into an unknown baudrate, there is a
 safety mechanism built-in. Tie the RX pin to ground and power up
 OpenLog. You should see the LEDs blink back and forth for 2 seconds,
 then blink in unison. Now power down OpenLog and remove the RX/GND
 jumper. OpenLog is now reset to 9600bps.

 Please note: The preloaded Optiboot serial bootloader is 0.5k, and
 begins at 0x7E00 (32,256). If the code is larger than 32,256 bytes,
 you will get verification errors during serial bootloading.

 STAT1 LED is sitting on PD5 (Arduino D5) - toggles when character is received
 STAT2 LED is sitting on PB5 (Arduino D13) - toggles when SPI writes happen

 LED Flashing errors @ 2Hz:
 No SD card - 3 blinks
 Baud rate change (requires power cycle) - 4 blinks

 OpenLog regularly shuts down to conserve power. If after 0.5 seconds
 no characters are received, OpenLog will record any unsaved
 characters and go to sleep. OpenLog will automatically wake up and
 continue logging the instant a new character is received.

 1.55mA idle
 15mA actively writing

 Input voltage on VCC can be 3.3 to 12V. Input voltage on RX-I pin
 must not exceed 6V. Output voltage on TX-O pin will not be greater
 than 3.3V. This may cause problems with some systems - for example
 if your attached microcontroller requires 4V minimum for serial
 communication (this is rare).

 OpenLog has progressed significantly over the past three years.
 Please see Changes.txt or GitHub for a full change log.

 v3.0 Re-write of core functions. Support under Arduino v1.0. Now
 supports full speed 57600 and 115200bps! Lower standby power.

 28354 bytes out of 32256.

 Be sure to check out https://github.com/nseidle/OpenLog/issues for
 issues that have been resolved up to this version.

 Update to new beta version of SerialPort lib from Bill Greiman.
 Update to Arduino v1.0. With Bill's library, we don't need to hack
 the HardwareSerial.cpp.  Re-wrote the append function. This function
 is the most important function and has the most affect on record
 accuracy. We are now able to record at 57600 and 115200bps at full
 blast! The performance is vastly improved.

 To compile v3.0 you will need Arduino v1.0 and Bill's beta library:
 http://beta-lib.googlecode.com/files/SerialLoggerBeta20120108.zip
 Unzip 'SerialPortBeta20120106.zip' and 'SdFatBeta20120108.zip' to
 the libraries directory and close and restart Arduino.

 Small stuff:
 Redirected all static strings to point to the new way in Arduino
 1.0: NewSerial.print(F("asdf")); instead of PgmPrint
 Figured out lower standby power from the low-power tutorial:
 http://www.sparkfun.com/tutorials/309
 Corrected #define TRUE to built-in supported 'true'
 Re-arranged some functions
 Migrating to Uno bootloader to get an additional 1500 bytes of program space
 Replumbed everything to get away from hardware UART
 Reduced # of sub directory support from 15 levels to 2 to allow for more RAM

 Wildcard remove is not yet supported in v3.0
 Wildcard ls is not yet supported in v3.0
 efcount and efinfo is not yet supported in v3.0

 Testing at 115200. First test with clean/empty card. The second
 test is with 193MB across 172 files on the microSD card.
 1GB: 333075/333075, 333075/333075
 8GB: 333075/333075, 333075/333075
 16GB: 333075/333075, 333075/333075
 The card with tons of files may have problems. Whenever possible,
 use a clean, empty, freshly formatted card.

 Testing at 57600. First test with clean/empty card. The second
 test is with 193MB across 172 files on the microSD card.
 1GB: 111075/111075, 111075/111075
 8GB: 111075/111075, 111075/111075
 16GB: 111075/111075, 111075/111075
 The card with tons of files may have problems. Whenever possible,
 use a clean, empty, freshly formatted card.

 v3.1 Fixed bug where entire log data is lost when power is lost.

 Added fix from issue 76: https://github.com/nseidle/OpenLog/issues/76
 Added support for verbose and echo settings recorded to the config
 file and EEPROM.
 Bugs 101 and 102 fixed with the help of pwjansen and wilafau - thank you!!

 Because of the new code to the cycle sensitive append_log loop,
 115200bps is not as rock solid as I'd like.
 I plan to correct this in the Light version.

 Added a maxLoop variable calculation to figure out how many bytes
 to receive before we do a forced file.sync.

 v3.11 Added freeMemory support for RAM testing.

 This was taken from: http://arduino.cc/playground/Code/AvailableMemory

 */
