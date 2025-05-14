#pragma once

#include "ui/ui_element.h"

class widgetTime : public ui_element
{
	public:
		void create(int16_t x, int16_t y, uint16_t color, TEXT_ALIGN alignment);

		void capture_clean_sprite() override;

		// Virtual Funcs
		bool redraw(uint8_t fade_amount) override;
		bool process_touch(touch_event_t touch_event) override;
		void slow_fade();

	protected:
		int16_t _adj_x = 0; // alignment adjusted draw pos x
		int16_t _adj_y = 0; // alignment adjusted draw pos y

		uint8_t current_fade = 0;
		bool fade_dir = true;

		bool calculate_text_size(bool forced = false);

		unsigned long next_update = 0;

		bool swap_sprite = false;

		std::string _time_string = "";
		std::string _date_string = "";

		uint16_t timeh;
		uint16_t dateh;
		uint16_t timew;
		uint16_t datew;

		bool has_data = false;
		bool should_redraw = false;
		bool is_setup = false;

		BB_SPI_LCD font_check;
};

extern widgetTime widget_time;