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
		psram_string owner = "";
		psram_string device_class = "";
		psram_string state_class = "";
		psram_string unit_of_measurement = "";
		psram_string sensor_value = "";
		psram_string value_type = "";
		psram_string min_value = "";
		psram_string max_value = "";
		psram_string description = "";
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

		psram_string get_sensor_value()
		{
			return (sensor_value + unit_of_measurement);
		}
};

class MQTT_Stuff
{
	public:
		// std::map<psram_string, std::vector<MQTT_Payload>, psram_allocator<MQTT_Payload>> mqtt_topic_payloads;

		std::map<
			psram_string,
			std::vector<MQTT_Payload, PsramAllocator<MQTT_Payload>>,
			std::less<psram_string>,
			PsramAllocator<std::pair<const psram_string, std::vector<MQTT_Payload, PsramAllocator<MQTT_Payload>>>>>
			mqtt_topic_payloads;

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
