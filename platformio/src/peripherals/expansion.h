#ifndef EXPANSION_H
#define EXPANSION_H

#include "Arduino.h"
#include <Melopero_BME280.h>
/*
Class to handle any libs and functionality added via the I2C Expansion port
*/

class Expansion
{
	public:
		void init_bme280();
		void stop_bme280();
		void update_data_bme280();
		float get_temp_bme280();
		float get_humidity_bme280();
		float get_pressure_bme280();
		bool bme280_available();

	private:
		int8_t bme_status = BME280_E_DEV_NOT_FOUND;
		Melopero_BME280 bme; // I2C on external expansion port

		float cached_temp = 0.0;
		float cached_humidity = 0.0;
		float cached_pressure = 0.0;
		unsigned long next_read = 0;
		uint8_t retry_counter = 0;
		bool skip_check = false;
};

extern Expansion expansion;

#endif // EXPANSION_H
