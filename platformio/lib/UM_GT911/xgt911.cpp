// SQUiXL GT911 touch controller
// https://squixl.io
//
// (c) 2025 Unexpected Maker

#include "xgt911.h"
#include "UM_LCA9555.h" // ioex support

// register addresses
static const uint16_t CMD_REG = 0x8040;
static const uint16_t REFRESH_RATE_REG = 0x8056;
static const uint16_t RES_X_REG = 0x8048;
static const uint16_t RES_Y_REG = 0x804A;
static const uint16_t TOUCH_POINTS_REG = 0x804C;
static const uint16_t MODULE_SWITCH1_REG = 0x804D;
static const uint16_t CONFIG_CHKSUM_REG = 0x80FF;
static const uint16_t CONFIG_FRESH_REG = 0x8100;
static const uint16_t POINT_DATA_START = 0x8150;
static const uint16_t DATA_BUFFER_REG = 0x814E;

void xGT911::begin(uint8_t rstPin, uint8_t irqPin, bool reverseAxis, uint8_t address, uint16_t width, uint16_t height, uint8_t touchPoints, bool reverseX, bool reverseY, bool sito, uint16_t refreshRate, touchCallback_t touchCallback)
{
	_wire = &Wire;
	_rstPin = rstPin;
	_irqPin = irqPin;
	_address = address;
	_width = width;
	_height = height;
	_touchPoints = touchPoints;
	_reverseX = reverseX;
	_reverseY = reverseY;
	_reverseAxis = reverseAxis;
	_sito = sito;
	_refreshRate = refreshRate;
	_touchCallback = touchCallback;

	// prepare reset pin via expander or direct
	ioex.pin_mode(_rstPin, OUTPUT, LOW);

	// perform hardware reset
	reset();

	// configure controller
	writeReg(RES_X_REG, _width);
	writeReg(RES_Y_REG, _height);
	writeReg(TOUCH_POINTS_REG, _touchPoints);
	uint8_t msw1 = (uint8_t)((_reverseY << 7) | (_reverseX << 6) | (_reverseAxis << 3) | (_sito << 2) | 0x01);
	writeReg(MODULE_SWITCH1_REG, msw1);
	uint16_t rr = (1000000UL) / (_refreshRate * 250);
	writeReg(REFRESH_RATE_REG, rr);
	writeReg(CMD_REG, (uint8_t)0x00);
	updateConfig();
}

void xGT911::reset()
{
	// drive reset low
	ioex.write(_rstPin, LOW);

	delay(10);

	// pull IRQ low
	pinMode(_irqPin, OUTPUT);
	digitalWrite(_irqPin, LOW);
	delay(50);

	// release reset
	ioex.write(_rstPin, HIGH);
	delay(100);

	// set IRQ as input
	pinMode(_irqPin, INPUT_PULLUP);
}

void xGT911::writeReg(uint16_t reg, uint8_t val)
{
	_wire->beginTransmission(_address);
	_wire->write(reg >> 8);
	_wire->write(reg & 0xFF);
	_wire->write(val);
	_wire->endTransmission();
}

void xGT911::writeReg(uint16_t reg, uint16_t val)
{
	_wire->beginTransmission(_address);
	_wire->write(reg >> 8);
	_wire->write(reg & 0xFF);
	// little-endian data
	_wire->write(val & 0xFF);
	_wire->write((val >> 8) & 0xFF);
	_wire->endTransmission();
}

uint8_t xGT911::readReg(uint16_t reg)
{
	_wire->beginTransmission(_address);
	_wire->write(reg >> 8);
	_wire->write(reg & 0xFF);
	_wire->endTransmission(false);
	_wire->requestFrom(_address, (uint8_t)1);
	return _wire->available() ? _wire->read() : 0;
}

void xGT911::readReg(uint16_t reg, uint8_t *buf, uint16_t len)
{
	_wire->beginTransmission(_address);
	_wire->write(reg >> 8);
	_wire->write(reg & 0xFF);
	_wire->endTransmission(false);
	_wire->requestFrom(_address, (uint8_t)len);
	for (uint16_t i = 0; i < len && _wire->available(); ++i)
	{
		buf[i] = _wire->read();
	}
}

void xGT911::updateConfig()
{
	uint8_t cfg[184];
	// config block begins at 0x8047
	readReg(RES_X_REG - 1, cfg, 184);
	uint16_t sum = 0;
	for (uint8_t i = 0; i < 184; ++i)
		sum += cfg[i];
	uint8_t chksum = (~sum) + 1;
	writeReg(CONFIG_CHKSUM_REG, chksum);
	writeReg(CONFIG_FRESH_REG, (uint8_t)0x01);
}

bool xGT911::readID(uint8_t *idBuffer)
{
	readReg(0x8140, idBuffer, 4);
	return true;
}

uint8_t xGT911::readPoints(uint16_t (*points)[4])
{
	uint8_t status = readReg(DATA_BUFFER_REG);
	uint8_t n = status & 0x0F;
	if (status & 0x80)
	{
		for (uint8_t i = 0; i < n && i < _touchPoints; ++i)
		{
			uint8_t data[8];
			readReg(POINT_DATA_START + i * 8, data, 8);
			points[i][0] = data[0] | (data[1] << 8);
			points[i][1] = data[2] | (data[3] << 8);
			points[i][2] = data[4] | (data[5] << 8);
			points[i][3] = data[6]; // lower byte is ID
		}
		writeReg(DATA_BUFFER_REG, (uint8_t)0);
	}
	return n;
}

xGT911 xtouch;
