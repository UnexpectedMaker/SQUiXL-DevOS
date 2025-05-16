#pragma once

#include "ui/ui_element.h"
#include "ui/icons/images/ui_icons.h"
#include "ui/ui_screen.h"

class ui_icon : public ui_element
{
	public:
		void create(int16_t x, int16_t y, int16_t target_w, int16_t target_h, const void *image, int image_size, bool fade = true);

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;

	protected:
		int16_t _target_w; // draw pos x
		int16_t _target_h; // draw pos y
		float _scale = 1.0;

		uint8_t current_fade = 0;
		bool fade_dir = true;

		BB_SPI_LCD _icon;
};