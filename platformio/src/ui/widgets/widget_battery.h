#pragma once

#include "ui/ui_element.h"
#include "peripherals/battery.h"

class widgetBattery : public ui_element
{
	public:
		void create(int16_t x, int16_t y, uint16_t color);
		void load_icons();

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		void capture_clean_sprite() override;
		bool process_touch(touch_event_t touch_event) override;

	private:
		bool icons_loaded = false;
		bool showSSID = false;

		BB_SPI_LCD battery_icons[5];
		BB_SPI_LCD wifi_icons[5];
};

extern widgetBattery widget_battery;