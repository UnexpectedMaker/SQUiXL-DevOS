// Author: Leonardo La Rocca

#include "Melopero_BME280.h"

void delay_us(uint32_t period, void *intf_ptr)
{
	delayMicroseconds(period);
}

int8_t i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr)
{
	I2CInterfaceData i2c_data = *((I2CInterfaceData *)intf_ptr);

	i2c_data.i2c->beginTransmission(i2c_data.i2c_address);
	i2c_data.i2c->write(reg_addr);
	uint8_t i2c_status = i2c_data.i2c->endTransmission();
	if (i2c_status != BME280_OK)
		return BME280_E_COMM_FAIL;

	uint32_t index = 0;

	while (len > 0)
	{
		// request bytes for reading
		// note: using 32 instead of BUFFER_LENGTH ...
		uint32_t request = min((uint32_t)32, len);
		i2c_data.i2c->requestFrom(i2c_data.i2c_address, request);

		// avaialable bytes for reading
		uint32_t available = i2c_data.i2c->available();
		if (available == 0)
			return BME280_E_COMM_FAIL;

		len -= available;
		while (available > 0)
		{
			data[index++] = i2c_data.i2c->read();
			available--;
		}
	}

	return BME280_OK;
}

int8_t i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr)
{
	I2CInterfaceData i2c_data = *((I2CInterfaceData *)intf_ptr);

	int8_t status = BME280_OK;

	i2c_data.i2c->beginTransmission(i2c_data.i2c_address);
	i2c_data.i2c->write(reg_addr);
	i2c_data.i2c->write(data, len);
	uint8_t i2c_status = i2c_data.i2c->endTransmission();
	if (i2c_status != 0)
		status = BME280_E_COMM_FAIL;

	return status;
}

Melopero_BME280::Melopero_BME280()
{
	// Empty :(
}

int8_t Melopero_BME280::init_device(uint8_t i2c_address, TwoWire &bus)
{
	// Setup device address (interface pointer)
	i2c_interface.i2c_address = i2c_address;
	i2c_interface.i2c = &bus;

	// Setup device struct
	dev.intf = BME280_I2C_INTF;
	dev.read = i2c_read;
	dev.write = i2c_write;
	dev.delay_us = delay_us;
	dev.intf_ptr = &i2c_interface;

	// Initialize the bme280
	int8_t rslt = bme280_init(&dev);

	return rslt;
}

int8_t Melopero_BME280::set_oversampling(uint8_t pressure_os, uint8_t temperature_os, uint8_t humidity_os)
{
	dev.settings.osr_p = pressure_os;
	dev.settings.osr_t = temperature_os;
	dev.settings.osr_h = humidity_os;

	uint8_t settings = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL;
	int8_t rslt = bme280_set_sensor_settings(settings, &dev);

	return rslt;
}

int8_t Melopero_BME280::set_filter_coefficient(uint8_t filter_coefficient)
{
	dev.settings.filter = filter_coefficient;

	uint8_t settings = BME280_FILTER_SEL;
	int8_t rslt = bme280_set_sensor_settings(settings, &dev);

	return rslt;
}

int8_t Melopero_BME280::update_data()
{
	int8_t rslt = BME280_OK;
	/* In forced mode, a single measurement is performed in accordance
	to the selected measurement and filter options. When the measurement
	is finished, the sensor returns to sleep mode and the measurement
	results can be obtained from the data registers. For a next
	measurement, forced mode needs to be selected again.*/
	rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, &dev);
	if (rslt != BME280_OK)
		return rslt;

	/*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
	 *  and the oversampling configuration. */
	uint32_t req_delay = bme280_cal_meas_delay(&(dev.settings));

	/* Wait for the measurement to complete and print data */
	dev.delay_us(req_delay, dev.intf_ptr);
	rslt = bme280_get_sensor_data(BME280_ALL, &data, &dev);

	return rslt;
}

double Melopero_BME280::get_pressure()
{
	return data.pressure;
}

double Melopero_BME280::get_temperature()
{
	return data.temperature;
}

double Melopero_BME280::get_humidity()
{
	return data.humidity;
}

int8_t Melopero_BME280::set_weather_monitoring_configuration()
{
	int8_t rslt = set_oversampling(BME280_OVERSAMPLING_1X, BME280_OVERSAMPLING_1X, BME280_OVERSAMPLING_1X);
	if (rslt != BME280_OK)
		return rslt;
	rslt = set_filter_coefficient(BME280_FILTER_COEFF_OFF);
	return rslt;
}

int8_t Melopero_BME280::set_indoor_navigation_configuration()
{
	int8_t rslt = set_oversampling(BME280_OVERSAMPLING_16X, BME280_OVERSAMPLING_2X, BME280_OVERSAMPLING_1X);
	if (rslt != BME280_OK)
		return rslt;
	rslt = set_filter_coefficient(BME280_FILTER_COEFF_16);
	return rslt;
}

int8_t Melopero_BME280::set_gaming_configuration()
{
	int8_t rslt = set_oversampling(BME280_OVERSAMPLING_4X, BME280_OVERSAMPLING_1X, BME280_NO_OVERSAMPLING);
	if (rslt != BME280_OK)
		return rslt;
	rslt = set_filter_coefficient(BME280_FILTER_COEFF_16);
	return rslt;
}
