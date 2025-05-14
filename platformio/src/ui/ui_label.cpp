#include "ui/ui_label.h"

static std::vector<ui_label *> labels;

void ui_label::create(int16_t x, int16_t y, const char *title, uint16_t color, const GFXfont *pFont, TEXT_ALIGN alignment)
{
	_x = x;
	_y = y;
	_title = title;
	_font = pFont;
	_c = color;

	_align = alignment;

	// We need the font set before we can calculate the size
	_sprite_content.setFreeFont(_font);
	// Calculating the text size in pixels also calculates the alignment x,y
	calculate_text_size(true);

	_sprite_content.createVirtual(_w, _h, NULL, true);
	_sprite_back.createVirtual(_w, _h, NULL, true);
	_sprite_mixed.createVirtual(_w, _h, NULL, true);

	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_mixed.getBuffer());
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

	// We lose the font assignment when we createVirtual above, or freeVirtual
	_sprite_content.setFreeFont(_font);
	_sprite_content.setTextColor(_c, -1);
	_sprite_content.setCursor(0, _h - 1);
	_sprite_content.print(_title.c_str());

	// squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_LCD);
}

void ui_label::move(int16_t x, int16_t y)
{
	squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_mixed, 1.0f, -1, DRAW_TO_LCD);
	_x = x;
	_y = y;

	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

	_sprite_content.setFreeFont(_font);
	_sprite_content.setTextColor(_c, -1);
	_sprite_content.setCursor(0, _h - 1);
	_sprite_content.print(_title.c_str());
}

bool ui_label::redraw(uint8_t fade_amount)
{
	if (fade_amount == 0)
	{
		// Serial.println("Skipping redraw due to 0 fade value");
		return false;
	}

	// This is busy if something else is drawing this
	if (is_busy)
	{
		Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (is_dirty || fade_amount == 0)
	{
		// Serial.printf("Redraw Label: %s is _adj_x: %d, _adj_y: %d, w: %d, h: %d, fade amount: %d\n", _title.c_str(), _adj_x, _adj_y, _w, _h, fade_amount);

		squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_mixed.getBuffer());
		squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

		_sprite_content.setFreeFont(_font);
		_sprite_content.setTextColor(_c, -1);
		_sprite_content.setCursor(0, _h - 1);
		_sprite_content.print(_title.c_str());
	}

	if (fade_amount < 32)
	{

		// Serial.printf("Redraw Label: %s is _sprite_content: %d, _sprite_back: %d, fade amount: %d\n", _title.c_str(), _sprite_content, _sprite_back, fade_amount);

		squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, fade_amount);
		squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_back, 1.0f, 0x0, DRAW_TO_LCD);
	}
	else
	{
		squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, 0x0, DRAW_TO_LCD);
		next_refresh = millis() + refresh_interval;
	}

	is_dirty = false;
	is_busy = false;

	return true;
}

void ui_label::update(const char *title)
{
	// This is busy if something else is drawing this
	// This is busy if something else is drawing this
	if (is_busy)
	{
		Serial.println("Can't set title, busy...");
		return;
	}

	is_busy = true;

	squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_mixed, 1.0f, -1, DRAW_TO_LCD);

	_title = title;

	if (calculate_text_size())
	{
		_sprite_content.freeVirtual();
		_sprite_back.freeVirtual();
		_sprite_mixed.freeVirtual();

		_sprite_content.createVirtual(_w, _h, NULL, true);
		_sprite_back.createVirtual(_w, _h, NULL, true);
		_sprite_mixed.createVirtual(_w, _h, NULL, true);
	}

	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_mixed.getBuffer());
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
	squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

	_sprite_content.setFreeFont(_font);
	_sprite_content.setTextColor(_c, -1);
	_sprite_content.setCursor(0, _h - 1);
	_sprite_content.print(_title.c_str());

	squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_LCD);

	is_busy = false;

	// return true;
}

void ui_label::slow_fade()
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

void ui_label::show(bool fade)
{
	if (fade)
	{
		current_fade = 0;
		while (current_fade <= 32)
		{
			squixl.lcd.blendSprite(&_sprite_content, &_sprite_mixed, &_sprite_back, current_fade);
			squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_back, 1.0f, 0x0, DRAW_TO_LCD);
			delay(25);
		}
		fade_dir = false;
	}
	else
	{
		squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_LCD);
	}
}

// Private

bool ui_label::calculate_text_size(bool forced)
{
	int16_t tempx;
	int16_t tempy;
	uint16_t tempw;
	uint16_t temph;

	bool changed = false;

	_sprite_content.getTextBounds(_title.c_str(), 0, 0, &tempx, &tempy, &tempw, &temph);

	if (tempw != _w || temph != _h || forced)
	{
		changed = true;

		_w = tempw;
		_h = temph;

		Serial.printf("Update Label: %s - New Width: %d, Height: %d\n", _title.c_str(), _w, _h);

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
	}

	return changed;
}
