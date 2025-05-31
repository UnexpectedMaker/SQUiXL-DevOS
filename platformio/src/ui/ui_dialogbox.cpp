#include "ui/ui_dialogbox.h"
#include "ui/ui_screen.h"

// Dialog box stuff
void ui_dialogbox::show(const char *title, const char *message, uint16_t width, uint16_t height, uint16_t back_color, uint16_t text_color)
{
	if (is_open)
		return;

	is_open = true;

	_title = title;
	_message = message;

	_w = width;
	_h = height;
	_b_col = back_color;
	_t_col = text_color;

	// // take a backup of whats on the screen before we show the dialog
	sprite_dialog_clean.createVirtual(_w, _h, NULL, true);
	squixl.lcd.readImage(240 - _w / 2, 240 - _h / 2, _w, _h, (uint16_t *)sprite_dialog_clean.getBuffer());
	// Out dialog content sprite
	sprite_dialog_content.createVirtual(_w, _h, NULL, true);
	sprite_dialog_content.fillScreen(TFT_MAGENTA);
	sprite_dialog_content.fillRoundRect(0, 0, _w, _h, 7, _b_col, DRAW_TO_RAM); // white will be our mask

	// Title and button text uses size 1 font
	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);
	if (_button_ok != "" || _button_cancel != "")
	{
		calc_button_positions();
	}

	sprite_dialog_content.setTextColor(_t_col, -1);
	sprite_dialog_content.setFreeFont(UbuntuMono_R[1]);

	// If we have a title, show it
	if (_title != "")
	{
		uint16_t len = _title.length() * char_width;
		sprite_dialog_content.setCursor(_w / 2 - len / 2, char_height + 10);
		sprite_dialog_content.print(_title.c_str());
	}

	// If we have a dialog message, split it into lines and show it
	if (_message != "")
	{
		sprite_dialog_content.setFreeFont(UbuntuMono_R[2]);
		// Cache the char specs and max chars per line and max lines for word wrapping
		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &char_width, &char_height);
		max_chars_per_line = int((_w - 20) / char_width); // includes padding for margins
		max_lines = 5;

		lines.clear();
		squixl.split_text_into_lines(_message.c_str(), max_chars_per_line, lines);

		// Darken message text a bit so it's not too overwhelming.
		sprite_dialog_content.setTextColor(darken565(_t_col, 0.2f), -1);

		uint16_t pos_y = char_height * 3 + char_height / 2;
		for (int i = 0; i < min((int)lines.size(), (int)max_lines); i++)
		{
			uint16_t len = lines[i].length() * char_width;
			sprite_dialog_content.setCursor(_w / 2 - len / 2, pos_y + char_height);
			sprite_dialog_content.print(lines[i]);
			pos_y += char_height + 8;
		}

		lines.clear();
	}

	sprite_dialog_content.setFreeFont(UbuntuMono_R[1]);
	// If we have an OK button, show it
	if (_button_ok != "")
	{
		sprite_dialog_content.fillRoundRect(button_x_ok, button_y, button_w, button_h, 6, lighten565(_b_col, 0.2f), DRAW_TO_RAM);
		sprite_dialog_content.drawRoundRect(button_x_ok, button_y, button_w, button_h, 6, lighten565(_b_col, 0.5f), DRAW_TO_RAM);
		sprite_dialog_content.setCursor(button_cursor_x_ok, button_cursor_y);
		sprite_dialog_content.print(_button_ok.c_str());
	}

	// If we have a CANCEL button, show it
	if (_button_cancel != "")
	{
		sprite_dialog_content.fillRoundRect(button_x_cancel, button_y, button_w, button_h, 6, lighten565(_b_col, 0.2f), DRAW_TO_RAM);
		sprite_dialog_content.drawRoundRect(button_x_cancel, button_y, button_w, button_h, 6, lighten565(_b_col, 0.5f), DRAW_TO_RAM);
		sprite_dialog_content.setCursor(button_cursor_x_cancel, button_cursor_y);
		sprite_dialog_content.print(_button_cancel.c_str());
	}

	audio.play_tone(500, 1);

	// squixl.lcd.blendSprite(&sprite_dialog_content, &squixl.lcd, &squixl.lcd, 32, TFT_MAGENTA);
}

void ui_dialogbox::set_button_ok(const char *button_title_ok, CallbackFunction callback_ok)
{
	_button_ok = button_title_ok;
	_callback_ok = callback_ok;
}
void ui_dialogbox::set_button_cancel(const char *button_title_cancel, CallbackFunction callback_cancel)
{
	_button_cancel = button_title_cancel;
	_callback_cancel = callback_cancel;
}

void ui_dialogbox::calc_button_positions()
{
	len_ok = _button_ok.length() * char_width;
	len_cancel = _button_cancel.length() * char_width;

	num_buttons = 0;
	if (len_ok > 0)
		num_buttons++;
	if (len_cancel > 0)
		num_buttons++;

	// Buttons positions are in sprite space, not screen space
	button_x_ok = padding;
	button_x_cancel = _w / 2 + padding;
	button_w = _w / 2 - padding * 2;
	button_h = char_height * 3;
	button_y = _h - button_h - padding;

	if (num_buttons == 1)
	{
		button_x_ok = _w / 2 - button_w / 2;
	}

	button_cursor_x_ok = button_x_ok + button_w / 2 - len_ok / 2;
	button_cursor_x_cancel = button_x_cancel + button_w / 2 - len_cancel / 2;
	button_cursor_y = button_y + char_height * 2;

	if (num_buttons == 1)
	{
		button_x_ok = _w / 2 - button_w / 2;
		button_x_cancel = 0;
		button_cursor_x_cancel = 0;
	}
}

void ui_dialogbox::close()
{
	if (!is_open)
		return;

	is_open = false;

	// Refresh the current screen to force cleanup of the dialogbox
	squixl.current_screen()->_sprite_content.drawSprite(240 - _w / 2, 240 - _h / 2, &sprite_dialog_clean, 1.0f, -1, DRAW_TO_RAM);

	squixl.current_screen()->refresh(true);

	sprite_dialog_content.freeVirtual();
	sprite_dialog_clean.freeVirtual();

	_button_cancel = "";
	_button_ok = "";
}

void ui_dialogbox::draw()
{
	squixl.current_screen()->_sprite_content.drawSprite(240 - _w / 2, 240 - _h / 2, &sprite_dialog_content, 1.0f, TFT_MAGENTA, DRAW_TO_RAM);
}

bool ui_dialogbox::check_button_hit(uint16_t x, uint16_t y)
{
	// This should not happen, but we dont want to lock people out
	if (num_buttons == 0)
		return true;

	// Calculate top-left of centered window
	const int window_x = (480 - _w) / 2; // 100
	const int window_y = (480 - _h) / 2; // 140

	// Map screen coords to window-local coords
	uint16_t adjusted_x = x - window_x;
	uint16_t adjusted_y = y - window_y;

	bool hit = false;

	// Serial.printf("adjusted %d,%d against button_y %d, _h %d\n", adjusted_x, adjusted_y, button_y, _h);

	if (adjusted_y >= button_y && adjusted_y < _h)
	{
		if (num_buttons == 1)
		{
			if (adjusted_x >= button_x_ok && adjusted_x < button_x_ok + button_w)
			{
				hit = true;
				if (_callback_ok != nullptr)
					_callback_ok();
			}
		}
		else if (num_buttons == 2)
		{
			if (adjusted_x >= button_x_ok && adjusted_x < button_x_ok + button_w)
			{
				hit = true;
				if (_callback_ok != nullptr)
					_callback_ok();
			}
			else if (adjusted_x >= button_x_cancel && adjusted_x < button_x_cancel + button_w)
			{
				hit = true;
				if (_callback_cancel != nullptr)
					_callback_cancel();
			}
		}
	}

	if (hit)
		close();

	return hit;
}

/*
			if (callbackFunction != nullptr)
				callbackFunction();

			audio.play_tone(500, 1);
			*/
ui_dialogbox dialogbox;
