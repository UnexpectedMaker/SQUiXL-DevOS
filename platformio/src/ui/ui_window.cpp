#include "ui/ui_window.h"

static std::vector<ui_window *> windows;

void ui_window::create(int16_t pos_x, int16_t pos_y, int16_t width, int16_t height, int16_t color, uint8_t transparency, uint8_t blur_count, const char *title)
{
	_x = pos_x;
	_y = pos_y;
	_w = width;
	_h = height;
	_c = color;
	_t = min(transparency, (uint8_t)32); // 0-32?
	_b = min(blur_count, (uint8_t)100);

	_title = title;
	_align = TEXT_ALIGN::ALIGN_LEFT;

	_font = UbuntuMono_R[1];

	// squixl.log_heap("UI window #1");

	_sprite_back.createVirtual(_w, _h, NULL, true);
	_sprite_content.createVirtual(_w, _h, NULL, true);
	_sprite_mixed.createVirtual(_w, _h, NULL, true);

	// Create the sprite to hold the clean background of the window
	// read whatever is on the screen alrady at x,y,w,h and store it in the sprite
	// we will draw over that with whatever is on the screen
	_sprite_clean.createVirtual(_w, _h, NULL, true);

	// squixl.log_heap("UI window #2");
	squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());

	// move(_x, _y, true);

	calculate_text_size(true);

	windows.push_back(this);

	is_dirty = true;
	is_dirty_hard = true;

	// Serial.printf("\nCreated window %s with transparency of %d - x: %d, y: %d, w: %d, h: %d\n\n", _title.c_str(), _t, _x, _y, _w, _h);

	// squixl.log_heap("UI window #3");
}

void ui_window::draw_window_heading()
{
	// Serial.println("Pre draw_window_heading");
	// Draw the window background and title bar
	_sprite_content.fillScreen(TFT_MAGENTA);
	_sprite_content.fillRoundRect(0, 0, _w, _h, 7, _c, DRAW_TO_RAM); // white will be our mask
	squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, _t, TFT_MAGENTA);
	_sprite_content.fillScreen(TFT_MAGENTA);
	_sprite_content.fillRoundRect(0, 0, _w, 24, 7, _c, DRAW_TO_RAM); // white will be our mask
	squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, min(_t * 2, 16), TFT_MAGENTA);

	// Serial.println("Mid draw_window_heading");

	// Draw the window title left jusified
	_sprite_back.setTextColor(TFT_WHITE, -1);
	_sprite_back.setFreeFont(UbuntuMono_R[1]);
	_sprite_back.setCursor(padding.left, _text_height + 7);
	_sprite_back.print(_title.c_str());

	// Serial.println("Post draw_window_heading");
}

// void ui_window::show(bool fade_in)
// {
// 	// capture clean backgrounds again
// 	squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
// 	squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());

// 	// DO we have any background blur to do?
// 	if (_b > 0)
// 	{
// 		// Serial.printf("Bluring with a count of %d\n", _b);
// 		for (int itter = 0; itter < _b; itter++)
// 		{
// 			_sprite_back.blurGaussian();
// 		}
// 	}

// 	// Do we have transparency?
// 	if (_t > 0)
// 	{
// 		squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

// 		// _sprite_content.fillScreen(TFT_BLACK, DRAW_TO_RAM);
// 		_sprite_content.fillRoundRect(0, 0, _w, _h, 7, _c, DRAW_TO_RAM); // white will be our mask

// 		squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, _t);

// 		_sprite_back.setTextColor(TFT_WHITE, -1);
// 		_sprite_back.setFreeFont(_font);
// 		int16_t x1, y1;
// 		_sprite_back.getTextBounds(_title.c_str(), _x, _y, &x1, &y1, &_text_width, &_text_height);
// 		_sprite_back.setCursor(_w / 2 - _text_width / 2, _text_height + 8);
// 		_sprite_back.print(_title.c_str());

// 		for (int w = 0; w < ui_children.size(); w++)
// 		{
// 			ui_children[w]->set_dirty(true);
// 			ui_children[w]->redraw(32);
// 		}

// 		if (fade_in)
// 		{
// 			for (uint8_t u8Alpha = 0; u8Alpha < 32; u8Alpha++)
// 			{
// 				squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, u8Alpha);
// 				squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
// 				delay(5);
// 			}
// 		}
// 		else
// 		{
// 			// squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, 32);
// 			squixl.lcd.drawSprite(_x, _y, &_sprite_back, 1.0f, 0x0, DRAW_TO_LCD);
// 		}
// 	}
// }

// void ui_window::move(int16_t pos_x, int16_t pos_y, bool fade_in)
// {
// 	// replace window background with clean sprite captures previously
// 	squixl.lcd.drawSprite(_x, _y, &_sprite_clean, 1.0f, -1, DRAW_TO_LCD);

// 	// Adjust the new x,y position
// 	_x = pos_x;
// 	_y = pos_y;

// 	show(fade_in);
// }

bool ui_window::redraw(uint8_t fade_amount)
{
	if (is_dirty_hard)
	{
		squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		is_dirty_hard = false;
	}

	if (is_dirty || fade_amount == 0)
	{
		squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_content.getBuffer());

		squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());

		// _sprite_content.fillScreen(TFT_BLACK, DRAW_TO_RAM);
		_sprite_content.fillRoundRect(0, 0, _w, _h, 7, _c, DRAW_TO_RAM); // white will be our mask

		squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, _t);

		_sprite_back.setTextColor(TFT_WHITE, -1);
		_sprite_back.setFreeFont(_font);
		_sprite_back.setCursor(_w / 2 - _text_width / 2, _text_height + 10);
		// Serial.printf("redraw() title: %s ||\n", _title.c_str());
		_sprite_back.print(_title.c_str());

		for (int w = 0; w < ui_children.size(); w++)
		{
			ui_children[w]->set_dirty(true);
			ui_children[w]->redraw(32);
		}
	}

	// // We dont want to fade back in to less transparency then we wanted when we created the window
	// if (fade_amount > _t)
	// 	return;

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, fade_amount);
		squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
	}
	else
	{

		// Serial.printf("Doing window child thingo om %d children\n", children.size());

		// squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, 32);
		squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
		next_refresh = millis() + refresh_interval;
	}

	is_dirty = false;

	return true;
}

bool ui_window::process_touch(touch_event_t touch_event)
{
	// Did any of my children recieve this touch event?
	for (int w = 0; w < ui_children.size(); w++)
	{
		if (ui_children[w]->process_touch(touch_event))
		{
			next_update = 0;
			return true;
		}
	}

	/*
		Do any logic here for my own touch requirements
	*/
	return false;
}

void ui_window::set_title_alignment(TEXT_ALIGN new_align)
{
	bool recalculate = (_align != new_align);
	_align = new_align;

	if (recalculate)
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

		is_dirty = true;
	}
}

// Private

bool ui_window::calculate_text_size(bool forced)
{
	int16_t tempx;
	int16_t tempy;
	uint16_t tempw;
	uint16_t temph;

	int _text_pos_x = _w / 2 - _text_width / 2;
	int _text_pos_y = _text_height;

	bool changed = false;

	_sprite_content.getTextBounds(_title.c_str(), _text_pos_x, _text_pos_y, &tempx, &tempy, &tempw, &temph);

	if (tempw != _w || temph != _h || forced)
	{
		changed = true;

		_text_width = tempw;
		_text_height = temph;

		// Serial.printf("Update Label: %s - New Width: %d, Height: %d, pos_x: %d, pos_y: %d\n", _title.c_str(), _text_width, _text_height, _text_pos_x, _text_pos_y);

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
