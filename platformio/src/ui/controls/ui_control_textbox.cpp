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

	if (!_sprite_content.getBuffer())
	{
		_sprite_content.createVirtual(_w, _h, NULL, true);
	}

	// Always get the latest value to ensure changes done externally (like web server) are reflected
	auto *opt = static_cast<SettingsOptionString *>(setting_option);
	_text = opt->get().c_str();

	// Clear the content sprite
	_sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);

	// Calculate the string pixel sizes to allow for text centering
	// This is only needed once
	// uses different font sizes based no string length
	if (char_width == 0 || string_len_pixels == 0)
	{
		uint8_t string_len = _text.length();
		if (string_len < 40)
			squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &char_width, &char_height);
		else
			squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);

		string_len_pixels = string_len * char_width;
	}

	_sprite_content.fillRoundRect(0, 0, _w, _h, 8, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1], DRAW_TO_RAM);
	_sprite_content.fillRoundRect(5, 20, _w - 10, 35, 6, static_cast<ui_screen *>(get_ui_parent())->dark_tint[3], DRAW_TO_RAM);

	_sprite_content.setTextColor(TFT_WHITE, -1);

	_sprite_content.setFreeFont(UbuntuMono_R[(_text.length() < 40 ? 2 : 1)]);
	_sprite_content.setCursor((_w / 2) - (string_len_pixels / 2), 40 + char_height / 2);
	_sprite_content.print(_text.c_str());

	// If the control has a title, show it at the top center
	if (_title.length() > 0)
	{
		_sprite_content.setFreeFont(UbuntuMono_R[0]);
		_sprite_content.setCursor((_w / 2) - (title_len_pixels / 2), char_height + 2);
		_sprite_content.setTextColor(static_cast<ui_screen *>(get_ui_parent())->light_tint[5], -1);
		_sprite_content.print(_title.c_str());
	}

	get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);

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
			// static_cast<ui_screen *>(get_ui_parent())->refresh(true);

			// if (callbackFunction != nullptr)
			// 	callbackFunction();

			// audio.play_tone(500, 1);

			// delay(10);
			// flash = false;
			// redraw(32);
			// static_cast<ui_screen *>(get_ui_parent())->refresh(true);

			return true;
		}
	}

	return false;
}

std::string ui_control_textbox::get_text()
{
	return _text;
}
