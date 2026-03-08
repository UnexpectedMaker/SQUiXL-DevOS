
#include "ui/controls/ui_control_tabgroup.h"
#include "ui/ui_screen.h"

void ui_control_tabgroup::set_tabs(std::vector<psram_string> names)
{
	for (int i = 0; i < names.size(); i++)
	{
		tab_names.push_back(names[i]);
		// Serial.printf("adding tab name %s at position %d\n", names[i].c_str(), i);
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
				if (parent_screen == nullptr)
				{
					parent_screen = static_cast<ui_screen *>(get_ui_parent());
				}
				parent_screen->clear_tabbed_children();
				current_tab = new_tab;
				parent_screen->clear_content();
				parent_screen->refresh(true, true);

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

	if (parent_screen == nullptr)
	{
		parent_screen = static_cast<ui_screen *>(get_ui_parent());
	}

	if (!_sprite_content.getBuffer())
	{
		_sprite_content.create(_w, _h, TFT_MAGENTA);
	}

	// _sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[0]);
	_sprite_content.setTextColor(TFT_WHITE, TFT_MAGENTA);

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 0, &char_width, &char_height);

	_sprite_content.fillRoundRect(5, 5, _w - 10, _h - 10, 5, parent_screen->dark_tint[4]);

	uint8_t num_tabs = tab_names.size();
	uint8_t tab_width = 470 / num_tabs;

	for (int i = 0; i < tab_names.size(); i++)
	{
		// If this tab is me, draw a rect behind the tab group name
		if (current_tab == i)
			_sprite_content.fillRoundRect(7 + i * tab_width, 7, tab_width - 4, _h - 14, 4, parent_screen->dark_tint[2]);

		_sprite_content.setTextColor(current_tab == i ? squixl.squixl_blue : parent_screen->light_tint[4], TFT_MAGENTA);
		uint16_t pos_x = 5 + i * tab_width + tab_width / 2 - (tab_names[i].length() * char_width) / 2;
		_sprite_content.setCursor(pos_x, _h / 2 + char_height / 2 - 2);
		_sprite_content.print(tab_names[i].c_str());
	}

	get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1);

	next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	// this is not a self updating element, so we never need to let the parent know its been update
	return false;
}