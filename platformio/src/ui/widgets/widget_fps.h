#pragma once

#include "ui/ui_element.h"

class widgetFPS : public ui_element
{
	public:
		void create(int16_t x, int16_t y, uint16_t color);
		void tick();

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;

	private:
		bool sprite_created = false;
		uint32_t loop_count = 0;
		unsigned long last_fps_time = 0;
		uint32_t fps = 0;
};
