#pragma once

#include "ui/ui_window.h"

class widgetBME280 : public ui_window
{
	public:
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event);

	private:
		unsigned long next_update = 0;

		bool is_setup = false;
		bool should_redraw = false;
		bool show_humidity = false;
};

extern widgetBME280 widget_bme280;