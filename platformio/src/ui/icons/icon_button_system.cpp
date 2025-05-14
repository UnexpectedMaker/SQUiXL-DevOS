#include "ui/icons/icon_button_system.h"

bool button_icon_system::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		Serial.printf("System Button bounds check touch %d,%d with %d,%d,%d,%d\n", touch_event.x, touch_event.y, _x, _y, _w, _h);
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (millis() - next_click_update > 500)
			{
				next_click_update = millis();
				// squixl.toggle_settings();
				Serial.println("System Button TAP");
				audio.play_tone(300, 2);

				// if (squixl.showing_settings)
				// 	squixl.settings_screen->animate_pos(Directions::UL, 250, tween_ease_t::EASE_OUT, nullptr);
				// // else
				// 	squixl._settings_screen->animate_pos(Directions::DR, 250, tween_ease_t::EASE_IN, nullptr);

				return true;
			}
		}
	}

	return false;
}

button_icon_system button_system;
