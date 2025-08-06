#include "ui/widgets/widget_bme280.h"
#include "peripherals/expansion.h"
#include "ui/ui_screen.h"

bool widgetBME280::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (millis() < delay_first_draw)
		return false;

	if (!is_setup)
	{
		// We have not initialised the BME280 or checked if it is even there....
		// If this fails, it wont check again, so BME280 needs to be connected on bootup
		is_setup = expansion.init_bme280();
		return false;
	}

	if (!expansion.bme280_available())
	{
		if (is_setup)
		{
			Serial.println("EXPANSION: BME280 Lost....");
			_sprite_back.fillScreen(TFT_MAGENTA);
			ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_back, 1.0f, -1, DRAW_TO_RAM);
			is_setup = false;
		}

		return false;
	}

	// if (!expansion.bme280_available())
	// 	return false;

	bool was_dirty = false;

	ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
	delay(10);
	_sprite_clean.fillScreen(TFT_MAGENTA);
	_sprite_clean.fillRoundRect(0, 0, _w, _h, 7, _c, DRAW_TO_RAM); // white will be our mask
	squixl.lcd.blendSprite(&_sprite_clean, &_sprite_back, &_sprite_back, _t, TFT_MAGENTA);

	_sprite_back.setTextColor(TFT_WHITE, -1);
	_sprite_back.setFreeFont(UbuntuMono_R[1]);
	_sprite_back.setCursor(padding.left, _text_height + 6);
	_sprite_back.print(_title.c_str());

	_sprite_back.setFreeFont(UbuntuMono_R[2]);

	// The BME might not be plugged in.. if it is, show the data, oherwise show not connected message
	if (expansion.bme280_available())
	{
		_sprite_back.setTextColor(TFT_YELLOW, -1);
		_sprite_back.setCursor(10, _h - 7);
		_sprite_back.print(expansion.get_temp_bme280());
		_sprite_back.print((settings.config.open_weather.units_metric ? "°C " : "°F "));
		if (show_humidity)
		{
			_sprite_back.setTextColor(TFT_CYAN, -1);
			_sprite_back.print(expansion.get_humidity_bme280());
			_sprite_back.print("%");
		}
		else
		{
			// Show pressure in hPA s it's an easier to read number
			_sprite_back.setTextColor(TFT_WHITE, -1);
			_sprite_back.print((expansion.get_pressure_bme280() / 100));
			_sprite_back.print(" hPa");
		}

		show_humidity = !show_humidity;
	}
	else
	{
		_sprite_back.setTextColor(TFT_RED, -1);
		_sprite_back.setCursor(10, _h - 7);
		_sprite_back.print("NO BME280 FOUND!");

		is_setup = false;
	}

	if (fade_amount < 32)
	{
		Serial.printf(">>>>>>> bme280 fade: %d\n", fade_amount);

		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, fade_amount);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, 32);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

		next_refresh = millis();
	}

	if (is_dirty && !was_dirty)
		was_dirty = true;

	is_dirty = false;
	is_busy = false;

	return (fade_amount < 32 || was_dirty);
}

bool widgetBME280::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			// if (millis() - next_click_update > 1000)
			// {
			// 	next_click_update = millis();
			// 	Serial.println("TAP");
			// 	audio.play_tone(300, 2);
			// 	stat++;
			// 	if (stat == 2)
			// 		stat = 0;
			// 	return true;
			// }
		}
	}

	return false;
}

// widgetBME280 widget_bme280;
