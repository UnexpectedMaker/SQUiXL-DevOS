#pragma once
#include "squixl.h"

class ui_gauge;

/*
{
	"owner": "xxxxxxx",
	"device_class": "temperature",
	"state_class": "measurement",
	"sensor_value": 23.90,
	"value_type": "float",
	"unit_of_measurement": "°C",
	"min_value": "-1.0",
	"max_value": "60.0",
	"description": "Office BME280",
	"timestamp": 999999999
}

*/

struct MQTT_Payload
{
		std::string owner = "";
		std::string device_class = "";
		std::string state_class = "";
		std::string unit_of_measurement = "";
		std::string sensor_value = "";
		std::string value_type = "";
		std::string min_value = "";
		std::string max_value = "";
		std::string description = "";
		int timestamp = 0;

		// Non serialised data
		bool is_dirty = true;
		ui_gauge *dash_item = nullptr;

		void upate_from(MQTT_Payload new_data)
		{
			if (sensor_value != new_data.sensor_value)
			{
				is_dirty = true;
			}
			sensor_value = new_data.sensor_value;
			timestamp = new_data.timestamp;
			is_dirty = true;
		}

		void set_dash_item(ui_gauge *new_dash_item)
		{
			if (dash_item != nullptr)
			{
				Serial.printf("%s %s already has a dash item!!!\n", owner, device_class);
				return;
			}
			dash_item = new_dash_item;
		}

		std::string get_sensor_value()
		{
			return (sensor_value + unit_of_measurement);
		}
};

class MQTT_Stuff
{
	public:
		std::map<std::string, std::vector<MQTT_Payload>> mqtt_topic_payloads;

		void mqtt_reconnect();
		void mqtt_callback(char *topic, byte *message, unsigned int length);
		void process_mqtt();

		// Static callback that wraps the instance method
		static void static_mqtt_callback(char *topic, byte *message, unsigned int length);

		bool mqtt_dirty = false;

	protected:
		unsigned long next_mqtt_reconnect = 0;

		bool is_mqtt_connecting = false;
		bool mqtt_server_setup = false;

		int8_t retry_attemps = 3;
		uint16_t retry_time = 5000;
};

extern MQTT_Stuff mqtt_stuff;
