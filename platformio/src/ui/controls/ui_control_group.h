#pragma once

#include "ui/controls/ui_control.h"

class ui_control_group : public ui_control
{
	public:
		bool redraw(uint8_t fade_amount) override;
		bool process_touch(touch_event_t touch_event) override;

		// Virtual funcs
		// void set_options_data(SettingsOptionBase *sett) {};
		void set_label_sizes() {};

	protected:
};
