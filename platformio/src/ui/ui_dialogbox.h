#pragma once

#include "squixl.h"
#include "fonts/ubuntu_mono_all_r.h"
#include "fonts/ubuntu_mono_all_b.h"
#include <vector>
#include <string>

class ui_dialogbox
{
		using CallbackFunction = void (*)();

	public:
		void show(const char *title = "", const char *message = "", uint16_t width = 240, uint16_t height = 200, uint16_t back_color = darken565(0x5AEB, 0.5), uint16_t text_color = TFT_WHITE);
		void set_button_ok(const char *button_title_ok = "ok", CallbackFunction callback_ok = nullptr);
		void set_button_cancel(const char *button_title_cancel = "cancel", CallbackFunction callback_cancel = nullptr);
		void close();

		bool is_active() { return is_open; }

		void draw();

		bool check_button_hit(uint16_t x, uint16_t y);

	private:
		void calc_button_positions();

		BB_SPI_LCD sprite_dialog_content, sprite_dialog_clean;

		bool is_open = false;

		std::string _title = "";
		std::string _message = "";

		uint16_t padding = 10;

		std::string _button_ok = "";
		CallbackFunction _callback_ok = nullptr;

		std::string _button_cancel = "";
		CallbackFunction _callback_cancel = nullptr;

		uint8_t num_buttons = 0;

		uint16_t button_x_ok = 0;
		uint16_t button_x_cancel = 0;
		uint16_t button_y = 0;
		uint16_t button_w = 0;
		uint16_t button_h = 0;
		uint16_t len_ok = 0;
		uint16_t len_cancel = 0;

		uint16_t button_cursor_x_ok = 0;
		uint16_t button_cursor_x_cancel = 0;
		uint16_t button_cursor_y = 0;

		uint8_t char_width = 0;
		uint8_t char_height = 0;
		uint8_t max_chars_per_line = 0;
		uint8_t max_lines = 0;

		uint16_t _w = 200;
		uint16_t _h = 180;

		uint16_t _b_col = darken565(0x5AEB, 0.5);
		uint16_t _t_col = TFT_WHITE;

		std::vector<String, psram_allocator<String>> lines;
};

extern ui_dialogbox dialogbox;