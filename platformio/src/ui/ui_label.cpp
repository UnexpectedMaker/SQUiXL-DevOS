#include "ui/ui_label.h"
#include "ui/ui_screen.h"

static std::vector<ui_label *> labels;

void ui_label::create(int16_t x, int16_t y, const char *title, uint16_t color, TEXT_ALIGN alignment)
{
	_x = x;
	_y = y;
	_title = title;
	_c = color;

	_align = alignment;

	// using the new directly stored char sizes
	_w = _title.length() * UbuntuMono_R_Char_Sizes[1][0];
	_h = UbuntuMono_R_Char_Sizes[1][1] + 4; // extra buffer just in case

	calculate_alignment();

	// We need the font set before we can calculate the size
	_sprite_back.createVirtual(_w, _h, NULL, true);
	_sprite_content.createVirtual(_w, _h, NULL, true);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(_c, -1);
}

bool ui_label::redraw(uint8_t fade_amount, int8_t tab_group)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (is_dirty || fade_amount == 0)
	{
		_sprite_content.fillScreen(TFT_MAGENTA);
		_sprite_content.setCursor(0, _h - 4);
		_sprite_content.print(_title.c_str());
	}

	if (fade_amount < 32)
	{

		squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, fade_amount);
		ui_parent->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_back, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		ui_parent->_sprite_content.drawSprite(_adj_x, _adj_y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
		next_refresh = millis() + refresh_interval;
	}

	is_dirty = false;
	is_busy = false;

	return true;
}

void ui_label::update(const char *title)
{
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
		_w = _title.length() * UbuntuMono_R_Char_Sizes[1][0];
		calculate_alignment();
		_sprite_back.freeVirtual();
		_sprite_back.createVirtual(_w, _h, NULL, true);
		_sprite_content.freeVirtual();
		_sprite_content.createVirtual(_w, _h, NULL, true);
		_sprite_content.setFreeFont(UbuntuMono_R[1]);
	}

	is_dirty = true;
	is_busy = false;
}

// Private

void ui_label::calculate_alignment()
{
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

	// Serial.printf("\n\n||||||   UI LABEL title calc - size is: %d,%d - %d,%d\n\n", _w, _h, _adj_x, _adj_y);
}
