#include "ui/controls/ui_control_textbox.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_screen.h"

void ui_control_textbox::set_options_data(SettingsOptionBase *sett)
{
	if (sett == nullptr)
		return;

	setting_option = sett;

	auto *opt = static_cast<SettingsOptionString *>(setting_option);
	_text = opt->get().c_str();
}

bool ui_control_textbox::redraw(uint8_t fade_amount, int8_t tab_group)
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
		uint8_t string_len = _text.length();
		string_len_pixels = string_len * char_width;
		// string_len_pixels = constrain(string_len_pixels, 0, _w);
		Serial.printf("string %s len %d, pixels %d, x %d, w %d, pos %d\n", _text.c_str(), string_len, string_len_pixels, _x, _w, (_w / 2) - (string_len_pixels / 2));
	}

	_sprite_content.fillRoundRect(0, 0, _w, _h, 8, squixl.current_screen()->dark_tint[1], DRAW_TO_RAM);
	_sprite_content.fillRoundRect(5, 20, _w - 10, 35, 6, squixl.current_screen()->dark_tint[3], DRAW_TO_RAM);

	_sprite_content.setTextColor(TFT_WHITE, -1);

	_sprite_content.setFreeFont(UbuntuMono_R[2]);
	_sprite_content.setCursor((_w / 2) - (string_len_pixels / 2), 40 + char_height / 2);
	_sprite_content.print(_text.c_str());

	// If the control has a title, show it at the top center
	if (_title.length() > 0)
	{
		_sprite_content.setFreeFont(UbuntuMono_R[0]);
		_sprite_content.setCursor((_w / 2) - (title_len_pixels / 2), char_height + 2);
		_sprite_content.setTextColor(squixl.current_screen()->light_tint[5], -1);
		_sprite_content.print(_title.c_str());
	}

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

void ui_control_textbox::set_text(const char *text)
{
	_text = text;
	if (setting_option)
	{
		auto *opt = static_cast<SettingsOptionString *>(setting_option);
		String new_text = String(text);
		opt->update(&new_text);
	}
	string_len_pixels = 0;
}

bool ui_control_textbox::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			keyboard.show(true, this);
			// flash = true;
			// redraw(32);
			// squixl.current_screen()->refresh(true);

			// if (callbackFunction != nullptr)
			// 	callbackFunction();

			// audio.play_tone(500, 1);

			// delay(10);
			// flash = false;
			// redraw(32);
			// squixl.current_screen()->refresh(true);

			return true;
		}
	}

	return false;
}

std::string ui_control_textbox::get_text()
{
	return _text;
}
