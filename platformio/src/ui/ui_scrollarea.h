#pragma once

#include "Arduino.h"
#include "ui/ui_element.h"
#include <string>

class ui_scrollarea : public ui_element
{
	public:
		void create(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, uint16_t color);
		void slice_sprites();

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event) override;
		virtual void about_to_show_screen() override;
		virtual void about_to_close_screen() override;

	protected:
		bool content_changed = true;
		DRAGGABLE drag_axis = DRAGGABLE::DRAG_NONE;
		int16_t _scroll_x = 0;
		int16_t _scroll_y = 0;
		int16_t _cached_scroll_x = 0;
		int16_t _cached_scroll_y = 0;

		int16_t content_sprite_width = 0;
		int16_t content_sprite_height = 0;

		int16_t acceleration_x = 0;
		int16_t acceleration_y = 0;
		bool momentum = false;

		int16_t content_width = 0;
		int16_t content_height = 0;

		float content_size_perc = 0.0;
		float _cached_content_size_perc = 0.0;

		int16_t scroll_x_min = 0;
		int16_t scroll_y_min = 0;

		uint8_t char_width = 0;
		uint8_t char_height = 0;

		BB_SPI_LCD _sprite_top;
		BB_SPI_LCD _sprite_bottom;
		BB_SPI_LCD _sprite_left;
		BB_SPI_LCD _sprite_right;

		void calculate_alignment();
};