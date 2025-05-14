/*
  Arduino Library for the LCA9555
  Copyright (c) 2025 Unexpected Maker
*/

#include "UM_LCA9555.h"

bool LCA9555::begin(uint8_t address, TwoWire *wire)
{
	if ((address < LCA9555_ADDR_MIN) || (address > LCA9555_ADDR_MAX))
	{
		return false;
	}
	_address = address;
	_wire = wire;
	_error = LCA9555_OK;
	// Invalidate cache
	memset(_cache_valid, 0, sizeof(_cache_valid));

	return connected();
}

bool LCA9555::connected()
{
	_wire->beginTransmission(_address);
	uint8_t rc = _wire->endTransmission();
	_error = (rc == 0 ? LCA9555_OK : LCA9555_I2C_ERROR);
	return (rc == 0);
}

uint8_t LCA9555::i2c_read(uint8_t reg)
{
	_wire->beginTransmission(_address);
	_wire->write(reg);
	if (_wire->endTransmission(false) != 0)
	{
		_error = LCA9555_I2C_ERROR;
		return 0;
	}
	_wire->requestFrom(_address, (uint8_t)1);
	if (_wire->available())
	{
		uint8_t v = _wire->read();
		_error = LCA9555_OK;
		return v;
	}
	_error = LCA9555_I2C_ERROR;
	return 0;
}

bool LCA9555::i2c_write(uint8_t reg, uint8_t value)
{
	_wire->beginTransmission(_address);
	_wire->write(reg);
	_wire->write(value);
	uint8_t rc = _wire->endTransmission();
	_error = (rc == 0 ? LCA9555_OK : LCA9555_I2C_ERROR);
	return (rc == 0);
}

uint8_t LCA9555::read_cached(uint8_t reg)
{
	if (!_cache_valid[reg])
	{
		_reg_cache[reg] = i2c_read(reg);
		_cache_valid[reg] = true;
	}
	return _reg_cache[reg];
}

bool LCA9555::pin_mode(uint8_t pin, uint8_t mode, uint8_t value)
{
	if (pin > 15)
	{
		_error = LCA9555_PIN_ERROR;
		return false;
	}
	uint8_t reg = (pin < 8 ? REG_CONFIG_PORT0 : REG_CONFIG_PORT1);
	uint8_t idx = pin & 0x07;

	uint8_t cfg = read_cached(reg);
	uint8_t mask = 1 << idx;
	if (mode == INPUT)
	{
		cfg |= mask;
	}
	else
	{
		cfg &= ~mask;
		// set output level first
		digitalWrite(pin, value);
	}
	if (cfg != _reg_cache[reg])
	{
		_reg_cache[reg] = cfg;
		_cache_valid[reg] = true;
		return i2c_write(reg, cfg);
	}
	return true;
}

bool LCA9555::write(uint8_t pin, uint8_t value)
{
	if (pin > 15)
	{
		_error = LCA9555_PIN_ERROR;
		return false;
	}
	uint8_t reg = (pin < 8 ? REG_OUTPUT_PORT0 : REG_OUTPUT_PORT1);
	uint8_t idx = pin & 0x07;

	uint8_t out = read_cached(reg);
	uint8_t mask = 1 << idx;
	if (value)
	{
		out |= mask;
	}
	else
	{
		out &= ~mask;
	}
	if (out != _reg_cache[reg])
	{
		_reg_cache[reg] = out;
		_cache_valid[reg] = true;
		return i2c_write(reg, out);
	}
	return true;
}

int LCA9555::read(uint8_t pin)
{
	if (pin > 15)
	{
		_error = LCA9555_PIN_ERROR;
		return LOW;
	}
	uint8_t reg = (pin < 8 ? REG_INPUT_PORT0 : REG_INPUT_PORT1);
	uint8_t idx = pin & 0x07;

	uint8_t v = i2c_read(reg);
	return (v & (1 << idx)) ? HIGH : LOW;
}

bool LCA9555::set_polarity(uint8_t pin, uint8_t polarity)
{
	if (pin > 15)
	{
		_error = LCA9555_PIN_ERROR;
		return false;
	}
	uint8_t reg = (pin < 8 ? REG_POLARITY_PORT0 : REG_POLARITY_PORT1);
	uint8_t idx = pin & 0x07;

	uint8_t pol = read_cached(reg);
	uint8_t mask = 1 << idx;
	if (polarity)
	{
		pol |= mask;
	}
	else
	{
		pol &= ~mask;
	}
	if (pol != _reg_cache[reg])
	{
		_reg_cache[reg] = pol;
		_cache_valid[reg] = true;
		return i2c_write(reg, pol);
	}
	return true;
}

uint8_t LCA9555::get_polarity(uint8_t pin)
{
	if (pin > 15)
	{
		_error = LCA9555_PIN_ERROR;
		return LOW;
	}
	uint8_t reg = (pin < 8 ? REG_POLARITY_PORT0 : REG_POLARITY_PORT1);
	uint8_t idx = pin & 0x07;

	uint8_t pol = read_cached(reg);
	return (pol & (1 << idx)) ? HIGH : LOW;
}

uint8_t LCA9555::last_error() const
{
	return _error;
}

LCA9555 ioex;
