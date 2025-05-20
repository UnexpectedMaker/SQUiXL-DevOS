#pragma once

#include "Arduino.h"
#include "ui/ui_element.h"
#include <string>

class ui_label : public ui_element
{
	public:
		void create(int16_t x, int16_t y, const char *title, uint16_t color, TEXT_ALIGN alignment = TEXT_ALIGN::ALIGN_CENTER);
		void update(const char *title);

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;

	protected:
		int16_t _adj_x; // alignment adjusted draw pos x
		int16_t _adj_y; // alignment adjusted draw pos y

		uint8_t char_width = 0;
		uint8_t char_height = 0;

		// std::string _title;
		const GFXfont *_font;

		uint8_t current_fade = 0;
		bool fade_dir = true;

		void calculate_text_size();
};