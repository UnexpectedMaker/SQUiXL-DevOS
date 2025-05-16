
#include "ui/controls/ui_control_tabgroup.h"
#include "ui/ui_screen.h"

void ui_control_tabgroup::set_tabs(std::vector<std::string> names)
{
	for (int i = 0; i < names.size(); i++)
	{
		tab_names.push_back(names[i]);
		Serial.printf("adding tab name %s at position %d\n", names[i].c_str(), i);
	}
}

bool ui_control_tabgroup::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			uint8_t num_tabs = tab_names.size();
			uint8_t tab_width = 470 / num_tabs;

			int8_t new_tab = touch_event.x / tab_width;

			// guard against the very edge if touch_x==470 or division truncation:
			if (new_tab >= num_tabs)
				new_tab = new_tab - 1;

			if (new_tab != current_tab)
			{
				current_tab = new_tab;

				// Serial.printf("New tab group tab is %d\n", current_tab);

				squixl.current_screen()->clear_content();
				squixl.current_screen()->refresh(true, true);

				audio.play_tone(500, 1);

				return true;
			}
		}
	}
	return false;
}

bool ui_control_tabgroup::redraw(uint8_t fade_amount, int8_t tab_group)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{
		_sprite_clean.fillScreen(TFT_MAGENTA);

		is_dirty_hard = false;
	}

	// Clear the content sprite
	_sprite_content.fillScreen(TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(TFT_WHITE, -1);

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);

	_sprite_content.fillRoundRect(5, 5, _w - 10, _h - 10, 5, squixl.current_screen()->dark_tint[4], DRAW_TO_RAM);

	uint8_t num_tabs = tab_names.size();
	uint8_t tab_width = 470 / num_tabs;

	for (int i = 0; i < tab_names.size(); i++)
	{
		// If this tab is me, draw a rect behind the tab group name
		if (current_tab == i)
			_sprite_content.fillRoundRect(6 + i * tab_width, 6, tab_width - 2, _h - 12, 5, squixl.current_screen()->dark_tint[2], DRAW_TO_RAM);

		_sprite_content.setTextColor(current_tab == i ? squixl.squixl_blue : squixl.current_screen()->light_tint[4], -1);
		uint16_t pos_x = i * tab_width + tab_width / 2 - (tab_names[i].length() * char_width) / 2;
		_sprite_content.setCursor(pos_x, _h / 2 + char_height / 2 - 2);
		_sprite_content.print(tab_names[i].c_str());
	}

	// Blend and draw the sprite to the current ui_screen content sprite
	squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
	// squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

	if (fade_amount == 32)
		next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	// this is not a self updating element, so we never need to let the parent know its been update
	return false;
}