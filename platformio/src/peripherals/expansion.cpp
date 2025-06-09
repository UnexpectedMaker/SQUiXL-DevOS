#include "expansion.h"

void Expansion::init_bme280()
{
	if (bme_status != BME280_OK)
	{
		bme_status = bme.init_device();

		if (bme280_available())
		{
			Serial.println("EXPANSION: Found BME280");
			bme.set_weather_monitoring_configuration();
			skip_check = false;
		}
	}
}

bool Expansion::bme280_available()
{
	if (skip_check)
		return false;

	if (bme_status == BME280_OK)
		return true;

	if (bme_status == BME280_E_NULL_PTR)
		Serial.println("EXPANSION: error: Null pointer");
	else if (bme_status == BME280_E_DEV_NOT_FOUND)
	{
		skip_check = true;
		Serial.println("EXPANSION: error: device not found");
	}
	else if (bme_status == BME280_E_INVALID_LEN)
		Serial.println("EXPANSION: error: invalid length");
	else if (bme_status == BME280_E_COMM_FAIL)
		Serial.println("EXPANSION: error: communication interface fail");
	else if (bme_status == BME280_E_SLEEP_MODE_FAIL)
		Serial.println("EXPANSION: error: sleep mode fail");
	else if (bme_status == BME280_E_NVM_COPY_FAILED)
		Serial.println("EXPANSION: error: NVM copy failed");
	else
		Serial.println("EXPANSION: Unknown error");

	retry_counter++;
	if (retry_counter == 3)
	{
		retry_counter = 0;
		init_bme280();
	}

	return false;
}

void Expansion::stop_bme280()
{
	if (bme_status)
	{
		bme_status = false;
	}
}

void Expansion::update_data_bme280()
{
	if (bme_status != BME280_OK)
	{
		Serial.println("No BME Found");
		return;
	}

	if (millis() - next_read > 1000)
	{
		bme_status = bme.update_data();
		next_read = millis();
		cached_temp = bme.get_temperature();
		cached_humidity = bme.get_humidity();
		cached_pressure = bme.get_pressure();
	}
}
float Expansion::get_temp_bme280()
{
	update_data_bme280();
	return cached_temp;
}
float Expansion::get_humidity_bme280()
{
	update_data_bme280();
	return cached_humidity;
}

float Expansion::get_pressure_bme280()
{
	update_data_bme280();
	return cached_pressure;
}

Expansion expansion;
