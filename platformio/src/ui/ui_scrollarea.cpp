#include "ui/ui_scrollarea.h"
#include "ui/ui_screen.h"

void ui_scrollarea::create(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, uint16_t color)
{
	_x = x;
	_y = y;
	_w = w;
	_h = h;
	_title = title;
	_c = color;

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);
	// // using the new directly stored char sizes
	// _w = _title.length() * UbuntuMono_R_Char_Sizes[1][0];
	// _h = UbuntuMono_R_Char_Sizes[1][1] + 4; // extra buffer just in case

	// calculate_alignment();

	// Background of the scroll area
	// _sprite_back.createVirtual(_w, _h, NULL, true);
	// Inner content of the scroll area
	_sprite_content.createVirtual(_w, _h, NULL, true);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(TFT_WHITE, -1);
}

void ui_scrollarea::set_scrollable(bool scroll_x, bool scroll_y)
{
	_can_scroll_x = scroll_x;
	_can_scroll_y = scroll_y;
}

bool ui_scrollarea::redraw(uint8_t fade_amount, int8_t tab_group)
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

		// _sprite_content.fillRoundRect(0, 0, _w, _h, 8, static_cast<ui_screen *>(get_ui_parent())->dark_tint[2], DRAW_TO_RAM);
		_sprite_content.setCursor(padding.left, char_height + 4);
		_sprite_content.print(_title.c_str());

		// _sprite_content.fillScreen(TFT_MAGENTA);
		// test to show the clipped scroll area
		_sprite_content.fillRoundRect(0, 20, _w - 20, _h - 20, 6, static_cast<ui_screen *>(get_ui_parent())->dark_tint[2]);

		// Scrollbar mockup
		_sprite_content.fillRect(_w - 10, 30, 10, _h - 40, static_cast<ui_screen *>(get_ui_parent())->dark_tint[3]);
		_sprite_content.fillRect(_w - 10, 30, 10, _h / 2, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1]);
	}

	if (fade_amount < 32)
	{
		// squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, fade_amount);
		// get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_back, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		// squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_back, 32, TFT_MAGENTA);
		get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
		next_refresh = millis() + refresh_interval;
		// Serial.printf("drawin scrollarea %s\n", get_title());
	}

	is_dirty = false;
	is_busy = false;

	return true;
}

bool ui_scrollarea::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == SCREEN_DRAG_H || touch_event.type == SCREEN_DRAG_V)
	{
		if (!is_dragging)
		{
			is_dragging = true;
			drag_axis = (touch_event.type == SCREEN_DRAG_H ? DRAGABLE::DRAG_HORIZONTAL : DRAGABLE::DRAG_VERTICAL);
		}
		_scroll_x = touch_event.x;
		_scroll_y = touch_event.y;

		if (_scroll_x != _cached_scroll_x || _scroll_y != _cached_scroll_y)
		{
			Serial.printf("scroll %d and %d\n", _scroll_x, _scroll_y);
			// draw_draggable();
		}
		_cached_scroll_x = _scroll_x;
		_cached_scroll_y = _scroll_y;

		is_dragging = true;
		return false;
	}
	else if (is_dragging)
	{
		// Reset drag and neighbours
		is_dragging = false;
		drag_axis = DRAGABLE::DRAG_NONE;
		return false;
	}

	return false;
}