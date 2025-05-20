#include "ui/ui_label.h"
#include "ui/ui_screen.h"

static std::vector<ui_label *> labels;

void ui_label::create(int16_t x, int16_t y, const char *title, uint16_t color, TEXT_ALIGN alignment)
{
	_x = x;
	_y = y;
	_title = title;
	_font = UbuntuMono_R[1];
	_c = color;

	_align = alignment;

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);

	calculate_text_size();

	// We need the font set before we can calculate the size
	_sprite_back.createVirtual(_w, _h, NULL, true);
	_sprite_content.createVirtual(_w, _h, NULL, true);

	_sprite_content.setFreeFont(_font);
	_sprite_content.setTextColor(_c, -1);
}

bool ui_label::redraw(uint8_t fade_amount, int8_t tab_group)
{
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

		// // squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_mixed.getBuffer());
		// squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		// squixl.lcd.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

		_sprite_content.fillScreen(TFT_MAGENTA);
		_sprite_content.setCursor(0, _h - 1);
		_sprite_content.print(_title.c_str());
	}

	if (fade_amount < 32)
	{

		squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, fade_amount);
		squixl.current_screen()->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_back, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		// squixl.lcd.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, 0x0, DRAW_TO_LCD);
		squixl.current_screen()->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
		next_refresh = millis() + refresh_interval;
	}

	// Serial.println("\n\n||||||   UI LABEL DRAW");

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

	bool changed_length = (strlen(title) != _title.length());
	_title = title;

	_sprite_content.fillScreen(TFT_MAGENTA);
	squixl.current_screen()->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);

	if (changed_length)
	{
		calculate_text_size();
		_sprite_back.freeBuffer();
		_sprite_back.createVirtual(_w, _h, NULL, true);
		_sprite_content.freeBuffer();
		_sprite_content.createVirtual(_w, _h, NULL, true);
		_sprite_content.setFreeFont(_font);
	}

	is_dirty = true;
	// redraw(32);

	is_busy = false;

	// return true;
}

// Private

void ui_label::calculate_text_size()
{
	_w = _title.length() * char_width;
	_h = char_height + 2;

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

	Serial.printf("\n\n||||||   UI LABEL title calc - size is: %d,%d - %d,%d\n\n", _w, _h, _adj_x, _adj_y);
}
