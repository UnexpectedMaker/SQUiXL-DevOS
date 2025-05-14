// SQUiXL GT911 touch controller
// https://squixl.io
//
// (c) 2025 Unexpected Maker

#ifndef GT911_H
#define GT911_H

#include <Arduino.h>
#include <Wire.h>

typedef void (*touchCallback_t)(void);

class xGT911
{
	public:
		// constructor
		xGT911() {};

		// initialize controller (call after Wire.begin())
		void begin(uint8_t rstPin, uint8_t irqPin, bool reverseAxis = false, uint8_t address = 0x5D, uint16_t width = 480, uint16_t height = 480, uint8_t touchPoints = 1, bool reverseX = true, bool reverseY = true, bool sito = true, uint16_t refreshRate = 240, touchCallback_t touchCallback = nullptr);

		// read 4-byte firmware/ID
		bool readID(uint8_t *idBuffer);

		// read touch points: points[i] = {x, y, size, id}
		// returns number of active points
		uint8_t readPoints(uint16_t (*points)[4]);

	private:
		TwoWire *_wire;
		uint8_t _address;
		uint8_t _rstPin;
		uint8_t _irqPin;
		uint16_t _width, _height;
		uint8_t _touchPoints;
		bool _reverseX, _reverseY, _reverseAxis, _sito;
		uint16_t _refreshRate;
		touchCallback_t _touchCallback;

		void reset();
		void writeReg(uint16_t reg, uint8_t val);
		void writeReg(uint16_t reg, uint16_t val);
		void updateConfig();
		uint8_t readReg(uint16_t reg);
		void readReg(uint16_t reg, uint8_t *buf, uint16_t len);
};

extern xGT911 xtouch;

#endif // GT911_H
