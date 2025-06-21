#pragma once

#include "ui/ui_window.h"
#include "ui/icons/images/weather/um_ow_01d.h"
#include "ui/icons/images/weather/um_ow_02d.h"
#include "ui/icons/images/weather/um_ow_03d.h"
#include "ui/icons/images/weather/um_ow_04d.h"
#include "ui/icons/images/weather/um_ow_09d.h"
#include "ui/icons/images/weather/um_ow_10d.h"
#include "ui/icons/images/weather/um_ow_11d.h"
#include "ui/icons/images/weather/um_ow_13d.h"
#include "ui/icons/images/weather/um_ow_50d.h"

#include "ui/icons/images/weather/um_ow_01n.h"
#include "ui/icons/images/weather/um_ow_02n.h"

class widgetOpenWeather : public ui_window
{
	public:
		// void draw(uint canvasid);
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event);

		std::string build_server_path();
		void load_icon(const String &name);

		void process_weather_data(bool success, const String &response);

	private:
		std::string server_path = "http://api.openweathermap.org/data/2.5/weather?q=";
		std::string geo_path = "http://api.openweathermap.org/geo/1.0/direct?q="; // example: http://api.openweathermap.org/geo/1.0/direct?q=melbourne,au&limit=1&appid=xxxxx
		std::string full_server_call = "";
		unsigned long next_update = 0;

		bool has_data = false;
		bool should_redraw = false;

		// Cached weather data
		int16_t _temp = -999;
		int16_t _humidity = 0;
		String _icon_name = "";
		String _icon_desc = "";
		String _weather_desc = "";
		uint8_t stat = 0;

		// Xtras for wider display
		int16_t _feels_like = -999;
		float _wind_speed = 0.0f;
		int16_t _pressure = 0;
		int16_t _sea_level = 0;
		int16_t _ground_level = 0;
		int _wind_dir = 0;
		std::string _sunrise = "";
		std::string _sunset = "";

		std::map<String, BB_SPI_LCD> ow_icons;
};

extern widgetOpenWeather widget_ow;