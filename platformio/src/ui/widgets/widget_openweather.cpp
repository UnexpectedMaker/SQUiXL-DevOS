#include "ui/widgets/widget_openweather.h"
#include "ui/ui_screen.h"

using json = nlohmann::json;

std::string widgetOpenWeather::build_server_path()
{
	if (settings.config.city == "" || settings.config.country == "" || settings.config.open_weather.enabled == false || settings.config.open_weather.api_key == "")
		return "";

	if (full_server_call == "")
		full_server_call = server_path + settings.config.city.c_str() + "," + settings.config.country.c_str() + "&APPID=" + settings.config.open_weather.api_key.c_str() + "&units=" + (settings.config.open_weather.units_metric ? "metric" : "imperial");

	return full_server_call;
}

void widgetOpenWeather::load_icons()
{
	// Serial.println("Loading OW icons");

	// Day Icons
	ow_icons["01d"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["01d"], 0, 0, um_ow_01d, sizeof(um_ow_01d));

	ow_icons["01n"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["01n"], 0, 0, um_ow_01n, sizeof(um_ow_01n));

	ow_icons["02d"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["02d"], 0, 0, um_ow_02d, sizeof(um_ow_02d));

	ow_icons["02n"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["02n"], 0, 0, um_ow_02n, sizeof(um_ow_02n));

	ow_icons["03"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["03"], 0, 0, um_ow_03d, sizeof(um_ow_03d));

	ow_icons["04"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["04"], 0, 0, um_ow_04d, sizeof(um_ow_04d));

	ow_icons["09"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["09"], 0, 0, um_ow_09d, sizeof(um_ow_09d));

	ow_icons["10"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["10"], 0, 0, um_ow_10d, sizeof(um_ow_10d));

	ow_icons["11"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["11"], 0, 0, um_ow_11d, sizeof(um_ow_11d));

	ow_icons["13"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["13"], 0, 0, um_ow_13d, sizeof(um_ow_13d));

	ow_icons["50"].createVirtual(64, 64, NULL, true);
	squixl.loadPNG_into(&ow_icons["50"], 0, 0, um_ow_50d, sizeof(um_ow_50d));

	Serial.println("OW icons loaded");
}

void widgetOpenWeather::process_weather_data(bool success, const String &response)
{
	bool ok = true;
	try
	{
		_temp = -999;

		// Serial.println(response);

		if (response == "ERROR")
		{
			next_update = 0;
			return;
		}

		json data = json::parse(response);

		json main = data["main"];
		if (main.is_object())
		{
			_temp = (uint16_t)(main.value("temp", 0));
			_humidity = (uint16_t)(main.value("humidity", 0));
			Serial.printf("Temp: %d, Humidity: %d\n", _temp, _humidity);
		}
		else
		{
			ok = false;
		}

		json weather = data["weather"];
		if (weather.is_array())
		{
			_icon_name = weather[0]["icon"];
			// we want to keep d/n names for 01 and 02, but for the rest we strip the n/d as we share day and night icons.
			if (_icon_name.substring(0, 2) != "01" && _icon_name.substring(0, 2) != "02")
				_icon_name = _icon_name.substring(0, 2);
			_icon_desc = weather[0]["description"];
			_weather_desc = weather[0]["main"];
			_weather_desc.toUpperCase();
			Serial.println(_icon_name + " - " + _icon_desc + "(" + _weather_desc + ")");
		}
		else
		{
			ok = false;
		}
	}
	catch (json::exception &e)
	{
		Serial.printf("response: %s\n", response);
		Serial.println("OW Json parse error:");
		Serial.println(e.what());
		next_update += 10000;

		ok = false;
	}

	has_data = ok;
	should_redraw = true;
}

bool widgetOpenWeather::redraw(uint8_t fade_amount)
{
	// we want the poll_frequency to be (mins in millis, so mins * 60 * 1000)
	if (millis() - next_update > (settings.config.open_weather.poll_frequency * 60000) || next_update == 0)
	{
		if (!icons_loaded)
		{
			icons_loaded = true;
			load_icons();

			next_update = 0;
		}
		else
		{

			// Let's see if we can get the data from openweather.org
			std::string url = build_server_path();
			// Serial.println("url " + url);
			if (!url.empty())
			{
				wifi_controller.add_to_queue(url, [this](bool success, const String &response) { this->process_weather_data(success, response); });
			}

			// deepsleep_data_stored = false;
			next_update = millis();
		}
	}

	bool was_dirty = false;

	if (fade_amount == 32 && should_redraw)
	{
		// squixl.lcd.drawSprite(_x, _y, &_sprite_clean, 1.0f, -1, DRAW_TO_LCD);
		_sprite_content.fillScreen(TFT_MAGENTA);
		squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);

		// Serial.printf("should_redraw? %d, fade_amount %d\n", should_redraw, fade_amount);
		should_redraw = false;
		is_dirty = true;
	}

	if (is_dirty || is_dirty_hard)
	{
		// Read the region of the clean screen wallpaper into the local clean sprite
		squixl.current_screen()->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		delay(10);
		_sprite_content.fillScreen(TFT_MAGENTA);
		_sprite_content.fillRoundRect(0, 0, _w, _h, 7, _c, DRAW_TO_RAM); // white will be our mask
		squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_back, _t, TFT_MAGENTA);

		_sprite_back.setTextColor(TFT_WHITE, -1);
		_sprite_back.setFreeFont(UbuntuMono_R[1]);
		_sprite_back.setCursor(padding.left, _text_height + 6);
		// Serial.printf("redraw() title: %s ||\n", _title.c_str());
		_sprite_back.print(_title.c_str());

		if (settings.config.country.length() > 0)
		{
			_sprite_back.setFreeFont(UbuntuMono_R[0]);
			_sprite_back.setTextColor(TFT_WHITE, -1);
			_sprite_back.setCursor(10, _h - 8);
			_sprite_back.print(settings.config.city);
			_sprite_back.print(", ");
			_sprite_back.print(settings.config.country);
		}

		// Serial.printf("is_dirty_hard? %d, is_dirty %d\n", is_dirty_hard, is_dirty);

		is_dirty_hard = false;
	}

	if (is_dirty)
	{
		_sprite_back.setFreeFont(UbuntuMono_R[2]);

		if (has_data)
		{
			_sprite_back.setFreeFont(UbuntuMono_R[1]);
			_sprite_back.setTextColor(TFT_CYAN, -1);
			_sprite_back.setCursor(10, 30);
			_sprite_back.print(_weather_desc.substring(0, 6));
			_sprite_back.setCursor(10, 47);
			_sprite_back.print(String(_humidity) + "%");
			_sprite_back.setFreeFont(UbuntuMono_R[4]);
			_sprite_back.setTextColor(TFT_YELLOW, -1);
			_sprite_back.setCursor(80, 46);
			_sprite_back.print(String(_temp) + (settings.config.open_weather.units_metric ? "°C" : "°F"));

			if (ow_icons.count(_icon_name) > 0)
			{
				_sprite_back.drawSprite((_w - 68), 4, &ow_icons[_icon_name], 1.0, 0x0, DRAW_TO_RAM);
			}

			// We've shown first data, so let's slow down the interval
			set_refresh_interval(30000);
		}
		else if (!settings.config.open_weather.enabled)
		{
			_sprite_back.setTextColor(TFT_RED - 1);
			_sprite_back.setCursor(10, 38);
			_sprite_back.print("NOT ENABLED!");
			Serial.println("NOT ENABLED!");
		}
		else if (!settings.config.open_weather.has_key())
		{
			_sprite_back.setTextColor(TFT_RED - 1);
			_sprite_back.setCursor(10, 38);
			_sprite_back.print("NO API KEY");
			Serial.println("NO API KEY");
		}
		else
		{
			_sprite_back.setTextColor(TFT_GREY, -1);
			_sprite_back.setCursor(10, 38);
			_sprite_back.print("WAITING...");
		}

		if (fade_amount < 32)
		{
			squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, fade_amount);
			squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		}
		else
		{
			squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, 32);
			squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

			// Serial.println("Tick weather");

			next_refresh = millis();
		}
	}

	if (is_dirty && !was_dirty)
		was_dirty = true;

	is_dirty = false;
	is_busy = false;

	return (fade_amount < 32 || was_dirty);
}

bool widgetOpenWeather::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();
				Serial.println("TAP");
				audio.play_tone(300, 2);
				stat++;
				if (stat == 2)
					stat = 0;
				return true;
			}
		}
	}

	return false;
}

widgetOpenWeather widget_ow;
