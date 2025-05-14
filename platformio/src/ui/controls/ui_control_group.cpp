
#include "ui/controls/ui_control_group.h"
#include "ui/ui_screen.h"

bool ui_control_group::process_touch(touch_event_t touch_event)
{
	return false;
}

bool ui_control_group::redraw(uint8_t fade_amount)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{
		_sprite_clean.fillScreen(TFT_MAGENTA);

		is_dirty_hard = false;
	}

	// Clear the content sprite
	_sprite_content.fillScreen(TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[2]);
	_sprite_content.setTextColor(TFT_WHITE, -1);

	_sprite_content.fillRoundRect(0, 0, _w, _h, 8, squixl.current_screen()->dark_tint[5], DRAW_TO_RAM);
	// _sprite_content.fillRoundRect(5, 20, _w / 2 - 5, 35, 6, squixl.current_screen()->dark_tint[3], DRAW_TO_RAM);

	// If the control has a title, show it at the top center
	if (_title.length() > 0)
	{
		_sprite_content.setFreeFont(UbuntuMono_R[0]);
		_sprite_content.setCursor(10, char_height_title + 10);
		_sprite_content.setTextColor(squixl.current_screen()->light_tint[5], -1);
		_sprite_content.print(_title.c_str());
	}

	Serial.printf("Grid element creation: %d, %d, %d, %d\n", _x, _y, _w, _h);
	Serial.printf("Grid element sprites: %d, %d, %d\n", _sprite_content.getBuffer(), _sprite_clean.getBuffer(), _sprite_mixed.getBuffer());

	// Blend and draw the sprite to the current ui_screen content sprite
	squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
	// squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

	if (fade_amount == 32)
		next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	// this is not a self updating element, so we never need to let the parent know its been update
	return false;
}