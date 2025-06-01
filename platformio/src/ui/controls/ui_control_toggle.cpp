#include "ui/controls/ui_control_toggle.h"
#include "ui/ui_screen.h"

void ui_control_toggle::set_options_data(SettingsOptionBase *sett)
{
	if (sett == nullptr)
		return;

	setting_option = sett;

	auto *opt = static_cast<SettingsOptionBool *>(setting_option);
	toggle_state = opt->get();
}

const char *ui_control_toggle::get_state_text()
{
	return (toggle_state ? toggle_text_on.c_str() : toggle_text_off.c_str());
}

bool ui_control_toggle::redraw(uint8_t fade_amount, int8_t tab_group)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (!_sprite_content.getBuffer())
	{
		_sprite_content.createVirtual(_w, _h, NULL, true);
		// _sprite_clean.createVirtual(_w, _h, NULL, true);
		// _sprite_mixed.createVirtual(_w, _h, NULL, true);
	}

	if (is_dirty_hard)
	{
		// _sprite_clean.fillScreen(TFT_MAGENTA);

		// Get the initial value from the setting, if one if set
		if (setting_option != nullptr)
			toggle_state = static_cast<SettingsOptionBool *>(setting_option)->get();

		// Calculate the string pixel sizes to allow for text centering
		// This is only needed once
		if (char_width == 0 || string_off_len_pixels == 0)
		{

			squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &char_width, &char_height);
			uint8_t string_len = toggle_text_off.length();
			string_off_len_pixels = string_len * char_width;
			string_len = toggle_text_on.length();
			string_on_len_pixels = string_len * char_width;
		}

		is_dirty_hard = false;
	}

	// Clear the content sprite
	_sprite_content.fillScreen(TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[2]);
	_sprite_content.setTextColor(TFT_WHITE, -1);

	_sprite_content.fillRoundRect(0, 0, _w, _h, 8, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1], DRAW_TO_RAM);
	_sprite_content.fillRoundRect(5, 20, _w / 2 - 5, 35, 6, static_cast<ui_screen *>(get_ui_parent())->dark_tint[3], DRAW_TO_RAM);

	// If the control has a title, show it at the top center
	if (_title.length() > 0)
	{
		_sprite_content.setFreeFont(UbuntuMono_R[0]);
		_sprite_content.setCursor((_w / 2) - (title_len_pixels / 2), char_height + 2);
		_sprite_content.setTextColor(static_cast<ui_screen *>(get_ui_parent())->light_tint[5], -1);
		_sprite_content.print(_title.c_str());
	}

	// Show the toggle state label
	_sprite_content.setCursor((_w / 2) + 5, 40 + char_height / 2);

	_sprite_content.setFreeFont(UbuntuMono_R[2]);
	_sprite_content.setTextColor(TFT_WHITE, -1);

	uint8_t toggle_half_way = (_w / 2 - 5);

	if (toggle_state)
		_sprite_content.fillRoundRect(_w / 4, 25, 30, 25, 6, static_cast<ui_screen *>(get_ui_parent())->light_tint[4], DRAW_TO_RAM);
	else
		_sprite_content.fillRoundRect(10, 25, 30, 25, 6, static_cast<ui_screen *>(get_ui_parent())->light_tint[0], DRAW_TO_RAM);

	_sprite_content.print(get_state_text());

	/*
	Button style toggle
	if (toggle_state)
	{
		_sprite_content.fillRoundRect(0, 0, _w, _h, 7, TFT_WHITE, DRAW_TO_RAM);
		_sprite_content.setTextColor(static_cast<ui_screen *>(get_ui_parent())->background_color(), -1);
		_sprite_content.setCursor((_w / 2) - (string_on_len_pixels / 2), _h / 2 + char_height / 2);
		_sprite_content.printf(toggle_text_on.c_str());
	}
	else
	{
		_sprite_content.drawRoundRect(0, 0, _w, _h, 7, TFT_WHITE, DRAW_TO_RAM);
		_sprite_content.setTextColor(TFT_WHITE, -1);
		_sprite_content.setCursor((_w / 2) - (string_off_len_pixels / 2), _h / 2 + char_height / 2);
		_sprite_content.printf(toggle_text_off.c_str());
	}
	*/

	// Blend and draw the sprite to the current ui_screen content sprite
	// squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
	get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);

	if (fade_amount == 32)
		next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	// this is not a self updating element, so we never need to let the parent know its been update
	return false;
}

void ui_control_toggle::set_toggle_text(const char *_text_off, const char *_text_on)
{
	toggle_text_off = _text_off;
	toggle_text_on = _text_on;
}

bool ui_control_toggle::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		// Serial.printf("checking bounds %d,%d for %s\n", touch_event.x, touch_event.y, get_title());
		if (check_bounds(touch_event.x, touch_event.y))
		{
			toggle_state = !toggle_state;
			if (setting_option != nullptr)
			{
				static_cast<SettingsOptionBool *>(setting_option)->update(toggle_state);
			}
			redraw(32);
			static_cast<ui_screen *>(get_ui_parent())->refresh(true);

			if (callbackFunction != nullptr)
				callbackFunction();

			audio.play_tone(500, 1);

			return true;
		}
	}

	return false;
}
