#pragma once

#include "ui/ui_element.h"

class widgetWiFiManager : public ui_element
{
	public:
		void create();
		void load_icons();

		// Virtual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		void capture_clean_sprite() override;
		bool process_touch(touch_event_t touch_event) override;

		void set_back_screen(ui_screen *screen) { back_screen = screen; };

	private:
		bool icons_loaded = false;
		BB_SPI_LCD wifi_icons[5];

		ui_screen *back_screen = nullptr;
};

extern widgetWiFiManager widget_wifimanager;