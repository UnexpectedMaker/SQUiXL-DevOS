#include "ui/controls/ui_control_Button.h"
#include "ui/ui_screen.h"

bool ui_control_button::redraw(uint8_t fade_amount, int8_t tab_group)
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

	// Calculate the string pixel sizes to allow for text centering
	// This is only needed once
	if (char_width == 0 || string_len_pixels == 0)
	{
		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &char_width, &char_height);
		uint8_t string_len = _title.length();
		string_len_pixels = string_len * char_width;
		// Serial.printf("string len %d, pixels %d, x %d, w %d, pos %d\n", string_len, string_len_pixels, _x, _w, (_w / 2) - (string_len_pixels / 2));
	}

	// uint16_t dark_shade = darken565(squixl.current_screen()->background_color(), 0.2f);
	// uint16_t light_shade = lighten565(squixl.current_screen()->background_color(), 0.4f);

	_sprite_content.setFreeFont(UbuntuMono_R[2]);
	_sprite_content.setTextColor(TFT_WHITE, -1);
	_sprite_content.setCursor((_w / 2) - (string_len_pixels / 2), _h / 2 + char_height / 2);

	if (flash)
	{
		_sprite_content.fillRoundRect(0, 0, _w, _h, 9, TFT_WHITE, DRAW_TO_RAM);
		_sprite_content.setTextColor(squixl.current_screen()->background_color(), -1);
	}
	else
	{
		_sprite_content.fillRoundRect(0, 0, _w, _h, 9, squixl.current_screen()->dark_tint[1], DRAW_TO_RAM);
		_sprite_content.drawRoundRect(0, 0, _w, _h, 9, squixl.current_screen()->light_tint[3], DRAW_TO_RAM);
		_sprite_content.setTextColor(TFT_WHITE, -1);
	}
	_sprite_content.print(_title.c_str());

	// Blend and draw the sprite to the current ui_screen content sprite
	squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);

	squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

	if (fade_amount == 32)
		next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	// this is not a self updating element, so we never need to let the parent know its been update
	return false;
}

void ui_control_button::set_button_text(const char *_text)
{
	_title = _text;
}

bool ui_control_button::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			flash = true;
			redraw(32);
			squixl.current_screen()->refresh(true);

			if (callbackFunction != nullptr)
				callbackFunction();

			audio.play_tone(500, 1);

			delay(10);
			flash = false;
			redraw(32);
			squixl.current_screen()->refresh(true);

			return true;
		}
	}

	return false;
}
