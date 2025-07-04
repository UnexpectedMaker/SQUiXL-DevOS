#include "ui/widgets/widget_jokes.h"

#include "ui/ui_screen.h"

using json = nlohmann::json;

void widgetJokes::reset_refresh_timer()
{
	process_next_joke = true;
	next_refresh = 0;
}

void widgetJokes::show_next_joke()
{
	if (stored_jokes.size() == 1 || !has_had_any_jokes)
	{
		if (!is_getting_more_jokes)
		{

			if (settings.has_wifi_creds() && !server_path.empty() && !wifi_controller.wifi_blocking_access)
			{
				is_getting_more_jokes = true;
				wifi_controller.add_to_queue(server_path, [this](bool success, const String &response) { this->process_joke_data(success, response); });
			}
		}
	}

	if (stored_jokes.size() > 0)
	{
		process_lines();
		fade(0.0, 1.0, 500, false, true, nullptr);
	}

	is_busy = false;
	next_refresh = millis() + 1000;
}

void widgetJokes::process_joke_data(bool success, const String &response)
{
	bool ok = true;
	try
	{
		// Serial.println(response);

		if (response == "ERROR")
		{
			next_update = 0;
			delete &response;
			ok = false;
			return;
		}

		json data = json::parse(response);

		if (data.is_array())
		{
			for (int i = 0; i < data.size(); i++)
				stored_jokes.push_back(JOKE(data[i]["setup"], data[i]["punchline"]));

			Serial.printf("Addded jokes, you now have %d\n\n", stored_jokes.size());
		}
		else
		{
			ok = false;
		}
	}
	catch (json::exception &e)
	{
		Serial.printf("response: %s\n", response.c_str());
		Serial.println("JOKES Json parse error:");
		Serial.println(e.what());
		next_update = 0;

		ok = false;
	}

	if (ok)
	{
		if (!has_had_any_jokes)
		{
			has_had_any_jokes = true;
			_sprite_joke.fillScreen(TFT_MAGENTA);
			// get rid of any waiting or other messages so we can draw the joke text
		}
		if (!is_getting_more_jokes)
		{
			// If we have local jokes stored now, we process the first one into word wrapped lines
			if (stored_jokes.size() > 0)
			{
				reset_refresh_timer();
			}
		}

		is_getting_more_jokes = false;
	}

	delete &response;
	is_dirty = ok;
}

bool widgetJokes::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (millis() < delay_first_draw)
		return false;

	bool was_dirty = false;

	if (!is_setup)
	{
		is_setup = true;
		has_had_any_jokes = false;
		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);

		max_chars_per_line = int((_w - 20) / char_width); // includes padding for margins
		max_lines = int((_h - 60) / char_height);		  // includes padding for margins ahd top heading

		_sprite_joke.createVirtual(_w, _h, NULL, true);

		// if (settings.has_wifi_creds() && !server_path.empty() && !wifi_controller.wifi_blocking_access)
		// {
		// 	wifi_controller.add_to_queue(server_path, [this](bool success, const String &response) { this->process_joke_data(success, response); });
		// }

		next_joke_swap = 0;
		return false;
	}

	if (!has_had_any_jokes && millis() - next_joke_swap > 10000)
	{
		next_joke_swap = millis();
		if (settings.has_wifi_creds() && !server_path.empty() && !wifi_controller.wifi_blocking_access)
		{
			wifi_controller.add_to_queue(server_path, [this](bool success, const String &response) { this->process_joke_data(success, response); });
		}

		return false;
	}

	if (process_next_joke)
	{
		process_next_joke = false;

		show_next_joke();
		return false;
	}

	if (is_busy)
	{
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{

		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		delay(10);

		draw_window_heading();

		_sprite_joke.fillScreen(TFT_MAGENTA);

		is_dirty_hard = false;
		is_aniamted_cached = false;
		was_dirty = true;
	}

	if (!has_had_any_jokes)
	{
		_sprite_joke.setFreeFont(UbuntuMono_R[2]);
		_sprite_joke.setTextColor(TFT_GREY, -1);
		_sprite_joke.setCursor(10, 38);
		_sprite_joke.print("WAITING...");
	}
	else if (!is_aniamted_cached)
	{

		if (lines.size() > 0)
		{
			// Serial.println("Building joke text");
			int16_t start_y = 42;
			// 	// canvas[canvasid].setTextDatum(TR_DATUM);
			// 	// _sprite_back.setFreeFont(RobotoMono_Regular[13]);
			_sprite_joke.setFreeFont(UbuntuMono_R[1]);
			_sprite_joke.setTextColor(TFT_CYAN, -1);

			for (int l = 0; l < min((uint8_t)lines.size(), max_lines); l++)
			{
				if (lines[l] == "*nl*")
				{
					start_y += 5;
					_sprite_joke.setTextColor(TFT_YELLOW, -1);
				}
				else
				{
					_sprite_joke.setCursor(10, start_y);
					_sprite_joke.print(lines[l]);
					start_y += 20;
				}
			}

			was_dirty = true;
			is_aniamted_cached = true;
		}
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_joke, &_sprite_back, &_sprite_mixed, fade_amount, TFT_MAGENTA);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_joke, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}

	if (is_dirty && !was_dirty)
		was_dirty = true;

	is_dirty = false;
	is_busy = false;

	next_refresh = millis();

	return (fade_amount < 32 || was_dirty);
}

bool widgetJokes::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (is_busy)
				return false;

			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();
				if (stored_jokes.size() > 1)
				{
					audio.play_tone(505, 12);
					stored_jokes.erase(stored_jokes.begin());
					// Serial.println("touched jokes");
				}

				next_refresh = millis() + 1000;
				fade(1.0, 0.0, 500, false, false, [this]() { reset_refresh_timer(); });
				return true;
			}
		}
	}

	return false;
}

void widgetJokes::process_lines()
{
	lines.clear();
	squixl.split_text_into_lines(stored_jokes[0].setup, max_chars_per_line, lines);
	lines.push_back("*nl*");
	squixl.split_text_into_lines(stored_jokes[0].punchline, max_chars_per_line, lines);

	// this is basically clearing the joke sprite
	_sprite_joke.fillScreen(TFT_MAGENTA);
	is_aniamted_cached = false;
}

widgetJokes widget_jokes;
