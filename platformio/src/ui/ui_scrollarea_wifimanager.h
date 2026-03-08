#pragma once
#include "ui/ui_scrollarea.h"

class ui_scrollarea_wifimanager : public ui_scrollarea
{
	public:
		bool process_touch(touch_event_t touch_event) override;
		void start_rescan();

	protected:
		bool external_content_dirty() const override;
		void render_content() override;
		void about_to_show_screen() override;

		void show_connected();

	private:
		int flash_index = -1;
		static constexpr int LINE_HEIGHT = 28;

		uint8_t wifi_char_width = 0;
		uint8_t wifi_char_height = 0;

		bool shown_connected = false;
};
