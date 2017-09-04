/* Serial LCD!
 * Uses 74HC595 shift register to control HD44780 LCDs in 4-bit mode.
 * MIT License © 2017 Daniel M. de Lima
 */
#pragma once

#include <Print.h>
#include <ShiftRegister.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYLEFT  0x02
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON  0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON  0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON  0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT  0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS  0x00

class SerialLCD : public Print {
	public:
		SerialLCD(ShiftRegister &io,
				uint8_t RS, uint8_t EN, uint8_t D4,
				uint8_t D5, uint8_t D6, uint8_t D7)
			: RS(RS), EN(EN), D4(D4), D5(D5), D6(D6), D7(D7),
			_io(io), _lines(), _offsets(), _func(), _mode(),
			_ctrl(LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF)
	{
		begin(16, 2);
	}
		void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS) {
			_lines = rows;
			if (_lines > 1) _func |= LCD_2LINE;
			setRowOffsets(0, 0x40, cols, 0x40+cols);
			if (charsize != LCD_5x8DOTS && _lines==1) _func = LCD_5x10DOTS;
			// LCD 4-bit mode power-up sequence (datasheet p.45-46)
			delayMicroseconds(50000);
			_io.write(RS, LOW);
			_io.write(EN, LOW);
			write4(3); delayMicroseconds(4500);
			write4(3); delayMicroseconds(4500);
			write4(3); delayMicroseconds(150);
			write4(2);
			// set LCD params and turn display on
			command(LCD_FUNCTIONSET | _func);
			display();
			clear();
			command(LCD_ENTRYMODESET | _mode);
		}

		void clear() {
			command(LCD_CLEARDISPLAY);
			delayMicroseconds(2000);
		}
		void home() {
			command(LCD_RETURNHOME);
			delayMicroseconds(2000);
		}

		void noDisplay() { command(LCD_DISPLAYCONTROL | (_ctrl &= ~LCD_DISPLAYON)); }
		void display()   { command(LCD_DISPLAYCONTROL | (_ctrl |=  LCD_DISPLAYON)); }
		void noBlink()   { command(LCD_DISPLAYCONTROL | (_ctrl &= ~LCD_BLINKON  )); }
		void blink()     { command(LCD_DISPLAYCONTROL | (_ctrl |=  LCD_BLINKON  )); }
		void noCursor()  { command(LCD_DISPLAYCONTROL | (_ctrl &= ~LCD_CURSORON )); }
		void cursor()    { command(LCD_DISPLAYCONTROL | (_ctrl |=  LCD_CURSORON )); }

		void scrollDisplayLeft()  { command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT ); }
		void scrollDisplayRight() { command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT); }
		void leftToRight()        { command(LCD_ENTRYMODESET | (_mode |=  LCD_ENTRYLEFT)); }
		void rightToLeft()        { command(LCD_ENTRYMODESET | (_mode &= ~LCD_ENTRYLEFT)); }
		void autoscroll()         { command(LCD_ENTRYMODESET | (_mode |=  LCD_ENTRYSHIFTINCREMENT)); }
		void noAutoscroll()       { command(LCD_ENTRYMODESET | (_mode &= ~LCD_ENTRYSHIFTINCREMENT)); }

		void setRowOffsets(int a, int b, int c, int d) {
			_offsets[0] = a; _offsets[1] = b;
			_offsets[2] = c; _offsets[3] = d;
		}
		void setCursor(uint8_t col, uint8_t row) {
			command(LCD_SETDDRAMADDR | (col + _offsets[row%_lines]));
		}
		void createChar(uint8_t addr, uint8_t bitmap[8]) {
			command(LCD_SETCGRAMADDR | (addr & 7 << 3));
			for (int i=0; i<8; i++) write(bitmap[i]);
		}
		virtual size_t write(uint8_t value) { send(value, HIGH); return 1; }
		void command(uint8_t value) { send(value, LOW); }

		using Print::write;
	private:
		void send(uint8_t value, uint8_t mode) {
			_io.write(RS, mode);
			write4(value>>4);
			write4(value);
		}
		void write4(uint8_t value) {
			_io.set(D4, bitRead(value, 0));
			_io.set(D5, bitRead(value, 1));
			_io.set(D6, bitRead(value, 2));
			_io.write(D7, bitRead(value, 3));
			pulseEnable();
		}
		void pulseEnable() {
			_io.write(EN, LOW);  delayMicroseconds(1);
			_io.write(EN, HIGH); delayMicroseconds(1);
			_io.write(EN, LOW);  delayMicroseconds(50);
		}

		ShiftRegister &_io;
		const uint8_t RS,EN,D4,D5,D6,D7;
		uint8_t _func, _ctrl, _mode, _lines;
		uint8_t _offsets[4];
};
