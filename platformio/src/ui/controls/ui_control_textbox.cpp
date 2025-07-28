#include "ui/controls/ui_control_textbox.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_screen.h"

void ui_control_textbox::set_options_data(SettingsOptionBase *sett)
{
	if (sett == nullptr)
		return;

	setting_option = sett;

	if (data_type == SettingsOptionBase::Type::FLOAT)
	{
		auto *opt = static_cast<SettingsOptionFloat *>(setting_option);
		_text = String(opt->get()).c_str();
	}
	else if (data_type == SettingsOptionBase::Type::INT)
	{
		auto *opt = static_cast<SettingsOptionInt *>(setting_option);
		_text = String(opt->get()).c_str();
	}
	else
	{
		auto *opt = static_cast<SettingsOptionString *>(setting_option);
		_text = opt->get().c_str();
	}
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
		char_width = 0;
		string_len_pixels = 0;
	}

	// Always get the latest value to ensure changes done externally (like web server) are reflected
	if (data_type == SettingsOptionBase::Type::FLOAT)
	{
		auto *opt = static_cast<SettingsOptionFloat *>(setting_option);
		_text = String(opt->get()).c_str();
	}
	else if (data_type == SettingsOptionBase::Type::INT)
	{
		auto *opt = static_cast<SettingsOptionInt *>(setting_option);
		_text = String(opt->get()).c_str();
	}
	else
	{
		auto *opt = static_cast<SettingsOptionString *>(setting_option);
		_text = opt->get().c_str();
	}

	// Clear the content sprite
	_sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);

	// Calculate the string pixel sizes to allow for text centering
	// uses different font sizes based on string length
	if (char_width == 0 || string_len_pixels == 0)
	{
		starting_size = 2;

		Serial.printf("Starting to calc font size and string for: %s\n", _text.c_str());
		num_vis_chars = -1;

		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, starting_size, &char_width, &char_height);
		uint8_t string_len = _text.length();
		string_len_pixels = string_len * char_width;

		Serial.printf("start: string_len_pixels: %d, w: %d using font size: %d\n", string_len_pixels, (_w - 10), starting_size);

		while (string_len_pixels > _w - 10 && starting_size > 0)
		{
			starting_size--;
			squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, starting_size, &char_width, &char_height);
			string_len_pixels = string_len * char_width;
			Serial.printf("string_len_pixels: %d, w: %d using font size: %d\n", string_len_pixels, (_w - 10), starting_size);
		}

		// if we are still wider than the sprite, even at font size 0, we need to chop the string up.
		if (string_len_pixels > _w - 10)
		{
			// calculate the number of visible chars, mins 3 so we can add ...
			num_vis_chars = ((_w - 10) / char_width) - 3;
			string_len_pixels = (num_vis_chars + 3) * char_width;

			// Serial.printf("NOPE! string_len_pixels: %d, w: %d, num_vis_chars: %d\n", string_len_pixels, (_w - 10), num_vis_chars);
		}
	}

	_sprite_content.fillRoundRect(0, 0, _w, _h, 8, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1], DRAW_TO_RAM);
	_sprite_content.fillRoundRect(5, 20, _w - 10, 35, 6, static_cast<ui_screen *>(get_ui_parent())->dark_tint[3], DRAW_TO_RAM);

	_sprite_content.setTextColor(TFT_WHITE, -1);

	_sprite_content.setFreeFont(UbuntuMono_R[starting_size]);
	_sprite_content.setCursor((_w / 2) - (string_len_pixels / 2), 37 + char_height / 2);

	// If num_visible_chars is -1, then the entire string fits, so just print it to the screen.
	if (num_vis_chars < 0)
		_sprite_content.print(_text.c_str());
	else
	{
		std::string visible_str = _text.substr(0, num_vis_chars) + "...";
		_sprite_content.print(visible_str.c_str());
	}

	// If the control has a title, show it at the top center
	if (_title.length() > 0)
	{
		if (char_height_title == 0)
		{
			squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 0, &char_width_title, &char_height_title);
		}
		_sprite_content.setFreeFont(UbuntuMono_R[0]);
		_sprite_content.setCursor((_w / 2) - (title_len_pixels / 2), char_height_title + 2);
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
		if (data_type == SettingsOptionBase::Type::FLOAT)
		{
			auto *opt = static_cast<SettingsOptionFloat *>(setting_option);
			opt->update(atof(text));
		}
		else if (data_type == SettingsOptionBase::Type::INT)
		{
			auto *opt = static_cast<SettingsOptionInt *>(setting_option);
			opt->update(atoi(text));
		}
		else
		{
			auto *opt = static_cast<SettingsOptionString *>(setting_option);
			String new_text = String(text);
			opt->update(&new_text);
		}
	}
	char_width = 0;
	string_len_pixels = 0;
}

bool ui_control_textbox::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			keyboard.show(true, this);
			return true;
		}
	}

	return false;
}

std::string ui_control_textbox::get_text()
{
	return _text;
}
