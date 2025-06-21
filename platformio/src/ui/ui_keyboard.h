#pragma once

#include "ui/controls/ui_control_textbox.h"
#include "fonts/ubuntu_mono_all_r.h"
#include <map>
#include <array>
#include <vector>
#include <cstdint>
#include <string>

class Keyboard
{
	public:
		// we’ve got 5 horizontal bands in the PNG (row 0 is your suggestion bar, we start real keys at row 1)
		static constexpr int ROWS = 5;

		// special codes above 0x0100 so they never collide with ASCII
		static constexpr uint16_t KC_SHIFT = 0x0100;
		static constexpr uint16_t KC_TOGGLE = 0x0101; // “?123” key
		static constexpr uint16_t KC_HIDE = 0x0102;	  // your “hide keyboard” icon
		static constexpr uint16_t KC_BACKSPACE = 0x0103;
		static constexpr uint16_t KC_RETURN = 0x0104;
		static constexpr uint16_t KC_SPACE = ' '; // ASCII space

		struct Key
		{
				uint16_t code;		  // ASCII for letters/digits, or one of the KC_ specials
				const char *label[2]; // what you draw on that key
				int16_t x, y;		  // top-left corner in the 480×280 sprite
				uint16_t w, h;		  // size
				bool draw;			  // draw on KB ?
		};

		Keyboard() { init_keymap(); }

		void show(bool state, ui_control_textbox *target);
		void redraw_kayboard();
		void update(touch_event_t touch);
		void print_text();
		bool showing = false;
		void animate(bool open);

		void move_cursor(uint16_t pos);
		void flash_cursor();
		void update_cursor();

		// call once (in ctor) to load every Key into _keys_by_row
		void init_keymap();

		// pass touch coords relative to sprite (0…479, 0…279)
		// returns nullptr if no key hit
		const Key *find_key_at(int tx, int ty) const;

	private:
		BB_SPI_LCD _sprite_keyboard, _sprite_background, _sprite_mixdown;

		int16_t _x = 0;	  // X position
		int16_t _y = 0;	  // Y position
		int16_t _w = 480; // Width
		int16_t _h = 280; // Height

		int16_t _kb_y = 480; // KB animate Y position

		uint8_t box_char_width = 0;
		uint8_t box_char_height = 0;

		uint8_t char_width = 0;
		uint8_t char_height = 0;

		uint8_t keychar_width = 0;
		uint8_t keychar_height = 0;

		uint8_t titlechar_width = 0;
		uint8_t titlechar_height = 0;

		bool cached_char_sizes = false;

		uint16_t touch_x = 0;
		uint16_t touch_y = 0;

		uint16_t string_start_pos_x = 0;
		uint16_t string_len_pixels = 0;
		uint8_t string_len = 0;
		uint16_t cursor_pos_x = 0;
		int16_t cursor_pos = -1;
		bool cursor_visible = true;
		bool can_flash = true;
		unsigned long next_cursor_flash = 0;
		uint16_t cursor_flash_speed = 500; // ms

		std::string _edited_text = "";

		uint8_t current_layout = 0;

		bool is_upper = false;
		bool is_numeric = false;

		ui_control_textbox *_target;

		std::array<std::vector<Key>, ROWS> _keys_by_row;
		std::array<std::vector<Key>, ROWS> _keys_by_row_numeric;
};

extern Keyboard keyboard;
