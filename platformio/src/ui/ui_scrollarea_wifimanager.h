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
		mutable bool wifi_results_dirty = false;

	private:
		void draw_flash_line(int index, bool flash);

		int flash_index = -1;
		static constexpr int LINE_HEIGHT = 28;

		bool shown_connected = false;
};
