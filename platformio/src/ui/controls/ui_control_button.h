#pragma once

#include "ui/controls/ui_control.h"

class ui_control_button : public ui_control
{
	public:
		bool redraw(uint8_t fade_amount) override;
		bool process_touch(touch_event_t touch_event) override;
		void set_button_text(const char *_text);

	protected:
		uint16_t string_len_pixels = 0;
		bool flash = false;
};
