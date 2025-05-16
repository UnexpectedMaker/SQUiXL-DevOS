#pragma once

#include "ui/controls/ui_control.h"

class ui_control_tabgroup : public ui_control
{
	public:
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event) override;

		void set_tabs(std::vector<std::string> names);
		void change_tab(int8_t tab) { current_tab = tab; }
		int8_t get_current_tab() { return current_tab; }

		// Virtual funcs
		// void set_options_data(SettingsOptionBase *sett) {};
		void set_label_sizes() {};

	protected:
		std::vector<std::string> tab_names;
		int8_t current_tab = 0;

		uint8_t char_width = 0;
		uint8_t char_height = 0;
};
