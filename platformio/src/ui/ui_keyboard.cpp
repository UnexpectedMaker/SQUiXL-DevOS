#include "ui/ui_keyboard.h"
#include "ui/ui_screen.h"
#include "ui/images/kboards/kb_empty.h"
#include "squixl.h"

void Keyboard::init_keymap()
{
	// clear any old data
	for (auto &r : _keys_by_row)
		r.clear();

	// clear any old data
	for (auto &r : _keys_by_row_numeric)
		r.clear();

	// ─── Row 1: "q w e r t y u i o p" ──────────────────────────────
	//   y=57…107 (height=51), 10 equal keys
	_keys_by_row[1] = {
		{'q', {"q", "Q"}, 22, 57, 39, 51, true},
		{'w', {"w", "W"}, 67, 57, 38, 51, true},
		{'e', {"e", "E"}, 111, 57, 38, 51, true},
		{'r', {"r", "R"}, 155, 57, 39, 51, true},
		{'t', {"t", "T"}, 200, 57, 38, 51, true},
		{'y', {"y", "Y"}, 244, 57, 39, 51, true},
		{'u', {"u", "U"}, 288, 57, 39, 51, true},
		{'i', {"i", "I"}, 333, 57, 38, 51, true},
		{'o', {"o", "O"}, 377, 57, 39, 51, true},
		{'p', {"p", "P"}, 421, 57, 39, 51, true},
	};

	_keys_by_row_numeric[1] = {
		{'q', {"1", "["}, 22, 57, 39, 51, true},
		{'w', {"2", "]"}, 67, 57, 38, 51, true},
		{'e', {"3", "{"}, 111, 57, 38, 51, true},
		{'r', {"4", "}"}, 155, 57, 39, 51, true},
		{'t', {"5", "#"}, 200, 57, 38, 51, true},
		{'y', {"6", "%"}, 244, 57, 39, 51, true},
		{'u', {"7", "^"}, 288, 57, 39, 51, true},
		{'i', {"8", "*"}, 333, 57, 38, 51, true},
		{'o', {"9", "+"}, 377, 57, 39, 51, true},
		{'p', {"0", "="}, 421, 57, 39, 51, true},
	};

	// ─── Row 2: "a s d f g h j k l" ─────────────────────────────
	//   y=114…164 (height=51), 9 equal keys
	_keys_by_row[2] = {
		{'a', {"a", "A"}, 44, 114, 39, 51, true},
		{'s', {"s", "S"}, 89, 114, 38, 51, true},
		{'d', {"d", "D"}, 133, 114, 39, 51, true},
		{'f', {"f", "F"}, 177, 114, 39, 51, true},
		{'g', {"g", "G"}, 222, 114, 38, 51, true},
		{'h', {"h", "H"}, 266, 114, 39, 51, true},
		{'j', {"j", "J"}, 311, 114, 38, 51, true},
		{'k', {"k", "K"}, 355, 114, 38, 51, true},
		{'l', {"l", "L"}, 399, 114, 39, 51, true},
	};

	_keys_by_row_numeric[2] = {
		{'a', {"-", "_"}, 44, 114, 39, 51, true},
		{'s', {"/", "\\"}, 89, 114, 38, 51, true},
		{'d', {":", "|"}, 133, 114, 39, 51, true},
		{'f', {";", "~"}, 177, 114, 39, 51, true},
		{'g', {"(", "<"}, 222, 114, 38, 51, true},
		{'h', {")", ">"}, 266, 114, 39, 51, true},
		{'j', {"$", "\x80"}, 311, 114, 38, 51, true},
		{'k', {"&", "\xA3"}, 355, 114, 38, 51, true},
		{'l', {"@", "\xA5"}, 399, 114, 39, 51, true},
	};

	// ─── Row 3: Shift, "z x c v b n m", Backspace ───────────────
	//   y=171…221 (height=51), 9 keys (first & last are wide)
	_keys_by_row[3] = {
		{KC_SHIFT, {"\xAE", "\xAF"}, 20, 171, 51, 51, true},
		{'z', {"z", "Z"}, 89, 171, 38, 51, true},
		{'x', {"x", "X"}, 133, 171, 39, 51, true},
		{'c', {"c", "C"}, 177, 171, 39, 51, true},
		{'v', {"v", "V"}, 222, 171, 38, 51, true},
		{'b', {"b", "B"}, 266, 171, 39, 51, true},
		{'n', {"n", "N"}, 311, 171, 38, 51, true},
		{'m', {"m", "M"}, 355, 171, 38, 51, true},
		{KC_BACKSPACE, {"⌫", "⌫"}, 409, 171, 51, 51, false},
	};

	_keys_by_row_numeric[3] = {
		{KC_SHIFT, {"#+=", "123"}, 20, 171, 51, 51, true},
		{'z', {"\"", "\""}, 89, 171, 38, 51, true},
		{'x', {".", "."}, 133, 171, 39, 51, true},
		{'c', {",", ","}, 177, 171, 39, 51, true},
		{'v', {"?", "?"}, 222, 171, 38, 51, true},
		{'b', {"!", "!"}, 266, 171, 39, 51, true},
		{'n', {"'", "'"}, 311, 171, 38, 51, true},
		{'m', {"\xB0", "\xB0"}, 355, 171, 38, 51, true},
		{KC_BACKSPACE, {"⌫", "⌫"}, 409, 171, 51, 51, false},
	};

	// ─── Row 4: "?123", Hide, Space, Return ──────────────────────
	//   y=228…277 (height=50), 4 keys
	_keys_by_row[4] = {
		{KC_TOGGLE, {"?123", "?123"}, 20, 228, 93, 50, true},
		{KC_SPACE, {"space", "space"}, 133, 228, 228, 50, true},
		{KC_RETURN, {"set", "set"}, 367, 228, 93, 50, true},
	};

	_keys_by_row_numeric[4] = {
		{KC_TOGGLE, {"abc", "ABC"}, 20, 228, 93, 50, true},
		{KC_SPACE, {"space", "space"}, 133, 228, 228, 50, true},
		{KC_RETURN, {"set", "set"}, 367, 228, 93, 50, true},
	};
}

const Keyboard::Key *Keyboard::find_key_at(int tx, int ty) const
{
	// scan each real row (1…4); row 0 is your suggestion bar
	for (int r = 1; r < ROWS; ++r)
	{
		auto &row = is_numeric ? _keys_by_row_numeric[r] : _keys_by_row[r];
		if (row.empty())
			continue;
		int y0 = row[0].y, h = row[0].h;
		if (ty >= y0 && ty < y0 + h)
		{
			for (auto &k : row)
			{
				if (tx >= k.x && tx < k.x + k.w && ty >= k.y && ty < k.y + k.h)
				{
					return &k;
				}
			}
		}
	}
	return nullptr;
}

void Keyboard::show(bool state, ui_control_textbox *target)
{

	if (state && !_sprite_keyboard.getBuffer())
	{
		_sprite_keyboard.createVirtual(480, 300, NULL, true);
		_sprite_background.createVirtual(480, 300, NULL, true);
		squixl.lcd.readImage(0, 180, 480, 300, (uint16_t *)_sprite_background.getBuffer());
		squixl.loadPNG_into(&_sprite_keyboard, 0, 0, kb_empty, sizeof(kb_empty));

		_sprite_keyboard.setFreeFont(UbuntuMono_R[2]);
		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &titlechar_width, &titlechar_height);
		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &char_width, &char_height);
		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 3, &keychar_width, &keychar_height);

		box_char_width = char_width;
		box_char_height = char_height;
	}

	if (state && target != nullptr)
	{
		audio.play_tone(1500, 0.5);
		_target = target;
		_edited_text = _target->get_text();

		// Serial.printf("Show Virtual KB? %d for %s\n", state, _target->get_title());
		showing = true;
		redraw_kayboard();
		can_flash = true;
	}
	else
	{
		_target = nullptr;
		showing = false;
		can_flash = false;
		cursor_visible = false;
		update_cursor();
		cursor_pos = -1;
		squixl.current_screen()->refresh(true, true);
		audio.play_tone(800, 0.5);

		if (_sprite_keyboard.getBuffer())
		{
			_sprite_keyboard.freeVirtual();
			_sprite_background.freeVirtual();
		}

		// squixl.log_heap("close KB");
	}
}

void Keyboard::update(touch_event_t t)
{
	if (!showing || !_target)
		return;
	int tx = t.x;
	int ty = t.y - 200; // because you blit the keyboard at y=200

	if (ty < 50)
	{
		// 1) clamp your touch X to the exact text‐bounds
		int x0 = string_start_pos_x;
		int x1 = string_start_pos_x + string_len_pixels;
		int cx = tx < x0
					 ? x0
					 : (tx > x1
							? x1
							: tx);

		// 2) convert into a 0-based “which character cell” index
		int rel = cx - x0;				 // 0…string_len_pixels
		int cell = rel / box_char_width; // 0…(string_len–1)

		// 3) cursor goes *after* that cell, so +1
		int idx = cell + 1;
		if (idx > string_len)
			idx = string_len;

		cursor_pos = idx;
		move_cursor(x0 + idx * box_char_width);
		audio.play_tone(900, 0.3);
	}
	else if (auto key = find_key_at(tx, ty))
	{
		// Serial.printf("Hit %s (code 0x%04X)\n", key->label[is_upper ? 1 : 0], key->code);
		audio.play_tone(1500, 0.3);
		switch (key->code)
		{
		case Keyboard::KC_BACKSPACE:
			// nothing to delete if we're already at the very start
			if (cursor_pos > 0)
			{
				if (cursor_pos == _edited_text.length())
				{
					// deleting the last character
					cursor_pos--;
					_edited_text = _edited_text.substr(0, cursor_pos);
				}
				else
				{
					// split at the cursor, drop the char before it, then rejoin
					std::string left = _edited_text.substr(0, cursor_pos - 1);
					std::string right = _edited_text.substr(cursor_pos);
					cursor_pos--;
					_edited_text = left + right;
				}
				print_text();
				// reposition pixel cursor: start + (char index * char width)
				cursor_pos_x = string_start_pos_x + cursor_pos * box_char_width;
			}

			break;
		case Keyboard::KC_RETURN:
			_target->set_text(_edited_text.c_str());
			// _target->redraw(32);
			show(false, nullptr);
			break;
		case Keyboard::KC_SHIFT:
			is_upper = !is_upper;
			redraw_kayboard();
			break;
		case Keyboard::KC_TOGGLE:
			is_numeric = !is_numeric;
			redraw_kayboard();
			break;
		case Keyboard::KC_HIDE:
			// hideKeyboard();
			break;
		case Keyboard::KC_SPACE:
		{
			// ensure cursor_pos is in range
			if (cursor_pos > _edited_text.length())
				cursor_pos = _edited_text.length();

			// insert a space at cursor_pos
			_edited_text.insert(cursor_pos, 1, ' ');
			cursor_pos++;

			print_text();
			// update pixel position of cursor
			cursor_pos_x = string_start_pos_x + cursor_pos * box_char_width;
		}
		break;

		default:
		{
			// printable character from your 2-entry label
			char c = key->label[is_upper ? 1 : 0][0];

			if (cursor_pos > _edited_text.length())
				cursor_pos = _edited_text.length();

			// insert the character at cursor_pos
			_edited_text.insert(cursor_pos, 1, c);
			cursor_pos++;

			print_text();
			cursor_pos_x = string_start_pos_x + cursor_pos * box_char_width;
		}
		break;
		}
	}
}

void Keyboard::redraw_kayboard()
{
	squixl.loadPNG_into(&_sprite_keyboard, 0, 0, kb_empty, sizeof(kb_empty));
	// set title
	std::string title = _target->get_title();
	int pixel_len = title.length() * titlechar_width;
	_sprite_keyboard.setFreeFont(UbuntuMono_R[1]);
	_sprite_keyboard.setTextColor(TFT_WHITE, -1);
	_sprite_keyboard.setCursor(240 - (pixel_len / 2), 5 + titlechar_height);
	_sprite_keyboard.print(_target->get_title());

	// setup keys
	_sprite_keyboard.setFreeFont(UbuntuMono_R[3]);
	_sprite_keyboard.setTextColor(TFT_WHITE, -1);

	for (int r = 1; r < ROWS; ++r)
	{
		auto &row = is_numeric ? _keys_by_row_numeric[r] : _keys_by_row[r];
		if (row.empty())
			continue;
		for (auto &k : row)
		{
			if (!k.draw)
				continue;

			std::string label = k.label[is_upper ? 1 : 0];
			// uint16_t pos_x = k.x;
			uint16_t pos_y = k.y + 20; // new offset for UI_control title

			uint8_t str_len = label.length();
			uint16_t pxls = str_len * keychar_width;

			_sprite_keyboard.setCursor(k.x + (k.w / 2) - (pxls / 2), pos_y + (k.h / 2) + (keychar_height / 2));
			// Serial.printf("adding key %s to KB\n", label.c_str());
			_sprite_keyboard.print(label.c_str());
		}
	}

	print_text();
}

void Keyboard::print_text()
{
	// 8,8,45,472

	string_len = _edited_text.length();

	if (string_len < 40)
	{
		box_char_width = char_width;
		box_char_height = char_height;
	}
	else
	{
		box_char_width = titlechar_width;
		box_char_height = titlechar_height;
	}

	string_len_pixels = string_len * box_char_width;
	string_start_pos_x = 240 - (string_len_pixels / 2);

	_sprite_keyboard.setFreeFont(UbuntuMono_R[(string_len < 40 ? 2 : 1)]);
	_sprite_keyboard.fillRect(10, 30, 460, 34, RGB(60, 60, 59));
	_sprite_keyboard.setTextColor(TFT_WHITE, -1);
	_sprite_keyboard.setCursor(string_start_pos_x, 46 + (box_char_height / 2));
	_sprite_keyboard.print(_edited_text.c_str());

	if (cursor_pos < 0)
	{
		cursor_pos = string_len;
		cursor_pos_x = string_start_pos_x + cursor_pos * box_char_width;
	}

	// _sprite_keyboard.setCursor(20, 26 + (char_height / 2));
	// for (int c = 128; c < 140; c++)
	// 	_sprite_keyboard.print((char)c);

	squixl.lcd.drawSprite(0, 180, &_sprite_keyboard, 1.0, -1);
}

void Keyboard::move_cursor(uint16_t pos)
{
	can_flash = false;
	cursor_visible = false;
	update_cursor();
	cursor_pos_x = pos;
	can_flash = true;
}

void Keyboard::update_cursor()
{
	int x = 240 - (string_len_pixels / 2) + cursor_pos_x - 1;
	x = cursor_pos_x - 1;
	squixl.lcd.drawRect(x, 200 + 12, 2, 30, cursor_visible ? TFT_BLUE : RGB(60, 60, 59));
}

void Keyboard::flash_cursor()
{
	if (millis() - next_cursor_flash > cursor_flash_speed)
	{
		next_cursor_flash = millis();
		if (can_flash)
		{
			cursor_visible = !cursor_visible;
			update_cursor();
		}
	}
}

Keyboard keyboard;