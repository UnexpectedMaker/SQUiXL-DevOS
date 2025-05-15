#pragma once

#include "ui/controls/ui_control.h"

class ui_control_toggle : public ui_control
{
	public:
		bool redraw(uint8_t fade_amount) override;
		bool process_touch(touch_event_t touch_event) override;
		void set_toggle_text(const char *_text_off, const char *_text_on);
		const char *get_state_text();

		void set_options_data(SettingsOptionBase *sett) override;

	protected:
		std::string toggle_text_off = "OFF";
		std::string toggle_text_on = "ON";
		bool toggle_state = false;
		uint16_t string_off_len_pixels = 0;
		uint16_t string_on_len_pixels = 0;
		bool flash = false;
};
