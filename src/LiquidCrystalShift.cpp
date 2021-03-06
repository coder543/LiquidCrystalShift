#include "LiquidCrystalShift.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystalShift constructor is called).

LiquidCrystalShift::LiquidCrystalShift(uint8_t serclk, uint8_t serdata, uint8_t latch)
{
  init(1, 255, serclk, serdata, latch);
}

void LiquidCrystalShift::init(uint8_t fourbitmode, uint8_t rw,
			 uint8_t serclk, uint8_t serdata, uint8_t latch)
{
  _rw_pin = rw;

  _rs_pin = 2;
  _enable_pin = 3;

	_serclk = serclk;
	_serdata = serdata;
	_latch = latch;

  if (fourbitmode)
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  else
    _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;

  // begin(16, 1);
}

void LiquidCrystalShift::shiftFlush()
{
  digitalWrite(_latch, LOW);
	shiftOut(_serdata, _serclk, LSBFIRST, _serialbuffer);
  digitalWrite(_latch, HIGH);
}

void LiquidCrystalShift::shiftSet(int pin, int value)
{
  if (value == LOW) {
    _serialbuffer &= ~(1 << pin);
  } else {
    _serialbuffer |= 1 << pin;
  }
}

void LiquidCrystalShift::shiftWrite(int pin, int value)
{
  shiftSet(pin, value);
  shiftFlush();
}

void LiquidCrystalShift::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;

  setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
  if (_rw_pin != 255) {
    pinMode(_rw_pin, OUTPUT);
  }
  pinMode(_serdata, OUTPUT);
  pinMode(_serclk, OUTPUT);
  pinMode(_latch, OUTPUT);


  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
  delayMicroseconds(50000);
  // Now we pull both RS and R/W low to begin commands
  shiftWrite(_rs_pin, LOW);
  shiftWrite(_enable_pin, LOW);
  if (_rw_pin != 255) {
    digitalWrite(_rw_pin, LOW);
  }

  //put the LCD into 4 bit or 8 bit mode
  if (! (_displayfunction & LCD_8BITMODE)) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms

    // second try
    write4bits(0x03);
    delayMicroseconds(4500); // wait min 4.1ms

    // third go!
    write4bits(0x03);
    delayMicroseconds(150);

    // finally, set to 4-bit interface
    write4bits(0x02);
  } else {
    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    command(LCD_FUNCTIONSET | _displayfunction);
    delayMicroseconds(4500);  // wait more than 4.1ms

    // second try
    command(LCD_FUNCTIONSET | _displayfunction);
    delayMicroseconds(150);

    // third go
    command(LCD_FUNCTIONSET | _displayfunction);
  }

  // finally, set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

}

void LiquidCrystalShift::setRowOffsets(int row0, int row1, int row2, int row3)
{
  _row_offsets[0] = row0;
  _row_offsets[1] = row1;
  _row_offsets[2] = row2;
  _row_offsets[3] = row3;
}

/********** high level commands, for the user! */
void LiquidCrystalShift::clear()
{
  command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LiquidCrystalShift::home()
{
  command(LCD_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LiquidCrystalShift::setCursor(uint8_t col, uint8_t row)
{
  const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
  if ( row >= max_lines ) {
    row = max_lines - 1;    // we count rows starting w/0
  }
  if ( row >= _numlines ) {
    row = _numlines - 1;    // we count rows starting w/0
  }

  command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystalShift::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalShift::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystalShift::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalShift::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystalShift::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystalShift::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystalShift::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystalShift::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystalShift::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystalShift::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystalShift::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystalShift::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystalShift::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystalShift::command(uint8_t value) {
  send(value, LOW);
}

inline size_t LiquidCrystalShift::write(uint8_t value) {
  send(value, HIGH);
  return 1; // assume sucess
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LiquidCrystalShift::send(uint8_t value, uint8_t mode) {
  shiftWrite(_rs_pin, mode);
  // if there is a RW pin indicated, set it low to Write
  if (_rw_pin != 255) {
    digitalWrite(_rw_pin, LOW);
  }

  if (_displayfunction & LCD_8BITMODE) {
    write8bits(value);
  } else {
    write4bits(value>>4);
    write4bits(value);
  }
}


void LiquidCrystalShift::pulseEnable(void) {
  pulse = true;
  shiftWrite(_enable_pin, LOW);
  delayMicroseconds(1);
  shiftWrite(_enable_pin, HIGH);
  delayMicroseconds(1);    // enable pulse must be >450ns
  shiftWrite(_enable_pin, LOW);
  pulse = false;
  delayMicroseconds(100);   // commands need > 37us to settle
}

void LiquidCrystalShift::write4bits(uint8_t value) {
	// digitalWrite(_latch, LOW);
	// shiftOut(_serdata, _serclk, LSBFIRST, value);
	// digitalWrite(_latch, HIGH);
  _serialbuffer = (_serialbuffer & 0x0F) | ((value << 4) & 0xF0);
  shiftFlush();

  pulseEnable();
}

void LiquidCrystalShift::write8bits(uint8_t value) {
  // for (int i = 0; i < 8; i++) {
  //   digitalWrite(_data_pins[i], (value >> i) & 0x01);
  // }

	// digitalWrite(_latch, LOW);
	// shiftOut(_serdata, _serclk, LSBFIRST, value);
  // digitalWrite(_latch, HIGH);
  // delayMicroseconds(100);
  // digitalWrite(_latch, LOW);

  // pulseEnable();
}
