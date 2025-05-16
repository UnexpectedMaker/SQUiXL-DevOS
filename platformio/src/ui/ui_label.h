#pragma once

#include "Arduino.h"
#include "ui/ui_element.h"
#include <string>

class ui_label : public ui_element
{
	public:
		void create(int16_t x, int16_t y, const char *title, uint16_t color, const GFXfont *pFont, TEXT_ALIGN alignment);
		void update(const char *title);
		void move(int16_t x, int16_t y);

		void show(bool fade = false);

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		void slow_fade();

	protected:
		int16_t _adj_x; // alignment adjusted draw pos x
		int16_t _adj_y; // alignment adjusted draw pos y

		// std::string _title;
		const GFXfont *_font;

		uint8_t current_fade = 0;
		bool fade_dir = true;

		bool calculate_text_size(bool forced = false);
};