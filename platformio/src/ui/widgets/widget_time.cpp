#include "ui/widgets/widget_time.h"
#include "ui/ui_screen.h"

void widgetTime::create(int16_t x, int16_t y, uint16_t color, TEXT_ALIGN alignment)
{
	_x = x;
	_y = y;
	_c = color;
	_w = 0;
	_h = 0;

	_align = alignment;

	// Cache the font sizes for the time and date for quick calcs
	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 5, &timew, &timeh);
	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &datew, &dateh);
}

void widgetTime::capture_clean_sprite()
{
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
	is_dirty_hard = false;
}

bool widgetTime::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (millis() < delay_first_draw)
		return false;

	unsigned long start_time = millis();

	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{
		// if (!_sprite_content.getBuffer())
		// {
		// 	_sprite_content.createVirtual(_w, _h, NULL, true);
		// 	_sprite_mixed.createVirtual(_w, _h, NULL, true);
		// 	_sprite_clean.createVirtual(_w, _h, NULL, true);
		// 	// delay(10);
		// }
		// // _sprite_content.fillScreen(TFT_MAGENTA);
		// _sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);
		// // _sprite_clean.fillScreen(TFT_MAGENTA);
		// _sprite_clean.fillRect(0, 0, _w, _h, TFT_MAGENTA);
		is_dirty_hard = false;
	}

	if (rtc.did_time_change() || !is_setup)
	{
		_time_string = rtc.get_time_string_seconds(true, true).c_str();
		_date_string = rtc.get_date_string(true, false).c_str();

		bool changed = false;
		if (_time_string_len != _time_string.length())
		{
			changed = true;
			_time_string_len = _time_string.length();
			_time_w_pixels = _time_string_len * timew;
		}
		if (_date_string_len != _date_string.length())
		{
			changed = true;
			_date_string_len = _date_string.length();
			_date_w_pixels = _date_string_len * datew;
		}

		if (changed)
		{
			uint16_t tempw = max(_time_w_pixels, _date_w_pixels);
			uint16_t temph = timeh + dateh + 10; // extra padding for 2 lines

			// Set new width and height for sprites
			if (tempw != _w || temph != _h)
			{
				_w = tempw;
				_h = temph;
			}

			// Work out new X,Y coordinates based on alingment
			if (_align == TEXT_ALIGN::ALIGN_LEFT)
			{
				_adj_x = _x;
				_adj_y = _y;
			}
			else if (_align == TEXT_ALIGN::ALIGN_CENTER)
			{
				_adj_x = _x - _w / 2;
				_adj_y = _y;
			}
			else if (_align == TEXT_ALIGN::ALIGN_RIGHT)
			{
				_adj_x = _x - _w;
				_adj_y = _y;
			}

			if (_sprite_content.getBuffer())
			{
				_sprite_content.freeVirtual();
			}

			_sprite_content.createVirtual(_w, _h, NULL, true);

			is_setup = true;
		}

		uint16_t max_width = max(_time_w_pixels, _date_w_pixels);
		uint8_t offset_x_time = max_width - _time_w_pixels;
		uint8_t offset_x_date = max_width - _date_w_pixels;

		_sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);

		_sprite_content.setFreeFont(UbuntuMono_R[5]);
		_sprite_content.setTextColor(_c, TFT_MAGENTA);
		_sprite_content.setCursor(offset_x_time, _h - dateh - 7);
		_sprite_content.print(_time_string.c_str());

		_sprite_content.setFreeFont(UbuntuMono_R[1]);
		_sprite_content.setCursor(offset_x_date, _h - 3);
		_sprite_content.print(_date_string.c_str());
	}

	// if (fade_amount < 32)
	// {
	// 	squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
	// 	ui_parent->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	// }
	// else
	// {
	ui_parent->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
	next_refresh = millis();
	// }

	// Serial.println("clock tick");

	is_dirty = false;
	is_busy = false;
	return true;
}

bool widgetTime::process_touch(touch_event_t touch_event)
{
	return false;
}

// widgetTime widget_time;