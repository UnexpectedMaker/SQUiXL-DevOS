#pragma once

#include "ui/ui_element.h"

class widgetSystem : public ui_element
{
	public:
		void create(int16_t x, int16_t y, uint16_t color);
		void load_icons();

		// Virtual Funcs
		bool redraw(uint8_t fade_amount) override;
		void capture_clean_sprite() override;
		bool process_touch(touch_event_t touch_event) override;

	private:
		bool icons_loaded = false;

		BB_SPI_LCD icon_system;
		BB_SPI_LCD icons_volume[4];
		// BB_SPI_LCD icons_brightness[3];
};

extern widgetSystem widget_system;