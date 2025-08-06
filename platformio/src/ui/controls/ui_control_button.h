#pragma once

#include "ui/controls/ui_control.h"

class ui_control_button : public ui_control
{
	public:
		static void *operator new(std::size_t size)
		{
			void *ptr = heap_caps_malloc(sizeof(ui_control_button), MALLOC_CAP_SPIRAM);
			if (!ptr)
				throw std::bad_alloc();
			return ptr;
		}

		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event) override;
		void set_button_text(const char *_text);

	protected:
		uint16_t string_len_pixels = 0;
		bool flash = false;
};
