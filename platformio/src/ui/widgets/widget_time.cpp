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
}

void widgetTime::capture_clean_sprite()
{
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
	is_dirty_hard = false;
}

bool widgetTime::redraw(uint8_t fade_amount, int8_t tab_group)
{

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
		if (!_sprite_content.getBuffer())
		{
			_sprite_content.createVirtual(_w, _h, NULL, true);
			_sprite_mixed.createVirtual(_w, _h, NULL, true);
			_sprite_clean.createVirtual(_w, _h, NULL, true);
			delay(10);
		}
		_sprite_content.fillScreen(TFT_MAGENTA);
		_sprite_clean.fillScreen(TFT_MAGENTA);
		is_dirty_hard = false;
	}

	if (rtc.did_time_change())
	{
		_time_string = rtc.get_time_string_seconds(true, true).c_str();
		_date_string = rtc.get_date_string(true, false).c_str();

		if (calculate_text_size(!is_setup))
		{
			// Serial.println("text calc size changed");

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

			if (_sprite_clean.getBuffer())
			{
				// Serial.println("closed sprites");
				_sprite_content.freeVirtual();
				_sprite_mixed.freeVirtual();
				_sprite_clean.freeVirtual();
			}

			_sprite_content.createVirtual(_w, _h, NULL, true);
			_sprite_mixed.createVirtual(_w, _h, NULL, true);
			_sprite_clean.createVirtual(_w, _h, NULL, true);
			delay(10);

			_sprite_content.fillScreen(TFT_MAGENTA);

			is_setup = true;
		}

		uint16_t max_width = max(timew, datew);
		uint8_t offset_x_time = max_width - timew;
		uint8_t offset_x_date = max_width - datew;

		_sprite_content.fillScreen(TFT_MAGENTA);

		_sprite_content.setFreeFont(UbuntuMono_R[5]);
		_sprite_content.setTextColor(_c, TFT_MAGENTA);
		_sprite_content.setCursor(offset_x_time, _h - dateh - 7);
		_sprite_content.print(_time_string.c_str());

		_sprite_content.setFreeFont(UbuntuMono_R[1]);
		_sprite_content.setCursor(offset_x_date, _h - 3);
		_sprite_content.print(_date_string.c_str());
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
		squixl.current_screen()->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		squixl.current_screen()->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
		// Serial.println("Tick time");
		next_refresh = millis();
	}

	// Serial.printf("time redraw time: %u ms\n", (millis() - start_time));

	is_dirty = false;
	is_busy = false;
	return true;
}

void widgetTime::slow_fade()
{
	if (fade_dir)
	{
		current_fade += 2;
		if (current_fade > 32)
		{
			current_fade = 32;
			fade_dir = !fade_dir;
		}
	}
	else
	{
		current_fade -= 2;
		if (current_fade < 2)
		{
			current_fade = 0;
			fade_dir = !fade_dir;
		}
	}

	squixl.lcd.blendSprite(&_sprite_content, &_sprite_mixed, &_sprite_back, current_fade);
	squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_back, 1.0f, 0x0, DRAW_TO_LCD);
}

// Private

bool widgetTime::calculate_text_size(bool forced)
{
	int16_t tempx;
	int16_t tempy;
	// uint16_t timew;
	// // uint16_t timeh;
	// uint16_t datew;
	// uint16_t dateh;

	bool changed = false;

	BB_SPI_LCD font_check;

	font_check.setFreeFont(UbuntuMono_R[5]);
	font_check.getTextBounds(_time_string.c_str(), 0, 0, &tempx, &tempy, &timew, &timeh);
	font_check.setFreeFont(UbuntuMono_R[1]);
	font_check.getTextBounds(_date_string.c_str(), 0, 0, &tempx, &tempy, &datew, &dateh);

	font_check.freeVirtual();

	uint16_t tempw = max(timew, datew);
	uint16_t temph = timeh + dateh + 10; // extra padding for 2 lines

	if (tempw != _w || temph != _h || forced)
	{
		changed = true;

		// if (_adj_x != 0 && _adj_y != 0)
		// {
		// 	// put the clean background back using the old with data
		// 	squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_clean, 1.0, -1, DRAW_TO_LCD);
		// }

		_w = tempw;
		_h = temph;

		// Serial.printf("Time Widget: %s - New Width: %d, Height: %d\n", _time_string.c_str(), _w, _h);
	}

	return changed;
}

bool widgetTime::process_touch(touch_event_t touch_event)
{
	return false;
}

widgetTime widget_time;