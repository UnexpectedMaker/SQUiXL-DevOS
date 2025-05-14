#pragma once

#include "ui/ui_element.h"
#include "fonts/ubuntu_mono_all_r.h"
#include <map>
#include <vector>
#include <string>

class ui_control_base;

class ui_widget : public ui_element
{
	public:
		using CallbackFunction = void (*)();

		void create(int16_t pos_x, int16_t pos_y, int16_t width, int16_t height, int16_t color, uint8_t transparecy, uint8_t blur_count, const char *title);

		void show(bool fade_in = false);

		void move(int16_t pos_x, int16_t pos_y, bool fade_in = false);

		// void set_title(const char *title = nullptr, TEXT_ALIGN align = ALIGN_CENTER, GFXfont *font = UbuntuMono_R[0]);

		// void set_title_alignment(TEXT_ALIGN align);
		// void set_title_font(GFXfont *font);
		// void set_title_alignment(TEXT_ALIGN align = ALIGN_CENTER);

		bool process_touch(touch_event_t touch_event) override;

	protected:
		uint16_t text_width, text_height;
		const GFXfont *_font = UbuntuMono_R[1];
};