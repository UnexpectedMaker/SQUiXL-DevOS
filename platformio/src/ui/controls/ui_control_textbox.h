#pragma once

#include "ui/controls/ui_control.h"

class ui_control_textbox : public ui_control
{
	public:
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event) override;
		void set_text(const char *text);
		std::string get_text();

		void set_data_type(SettingsOptionBase::Type _type) { data_type = _type; }
		void set_options_data(SettingsOptionBase *sett) override;

	protected:
		std::string _text = "";
		uint16_t string_len_pixels = 0;
		uint8_t cursor_char = 0;
		bool cursor_flash = false;
		SettingsOptionBase::Type data_type = SettingsOptionBase::Type::STRING;
};
