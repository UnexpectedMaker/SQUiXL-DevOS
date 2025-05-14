#pragma once

#include "ui/ui_window.h"
#include "ui/dashboard/ui_gauge_bme280.h"
// #include "ui/dashboard/ui_gauge_humidity.h"

class widgetMQTTSensors : public ui_window
{
	public:
		bool redraw(uint8_t fade_amount) override;
		bool process_touch(touch_event_t touch_event);

		void process_joke_data(bool success, const String &response);

	private:
		unsigned long next_update = 0;

		bool has_data = false;
		bool should_redraw = true;
		bool is_setup = false;

		uint16_t sensor_x = 9;
		uint16_t sensor_y = 30;

		std::vector<ui_gauge> dashboard_items;

		BB_SPI_LCD test_sprite;
		BB_SPI_LCD test_sprite_back;
};

extern widgetMQTTSensors widget_mqtt_sensors;