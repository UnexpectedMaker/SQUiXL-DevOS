#pragma once

#include "Arduino.h"
#include "ui/ui_element.h"
#include <string>

class ui_scrollarea : public ui_element
{
	public:
		void create(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, uint16_t color);
		void set_scrollable(bool scroll_x, bool scroll_y);

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event) override;

	protected:
		bool _can_scroll_x = false;
		bool _can_scroll_y = true;
		bool is_dragging = false;
		DRAGABLE drag_axis = DRAGABLE::DRAG_NONE;
		int16_t _scroll_x = 0;
		int16_t _scroll_y = 0;
		int16_t _cached_scroll_x = 0;
		int16_t _cached_scroll_y = 0;

		uint8_t char_width = 0;
		uint8_t char_height = 0;

		std::vector<ui_element *> content;

		void calculate_alignment();
};