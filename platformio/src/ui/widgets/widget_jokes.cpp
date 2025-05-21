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
	if (stored_jokes.size() == 1)
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
		// Serial.println("Show Next Joke");
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
			return;
		}

		json data = json::parse(response);

		if (data.is_array())
		{
			for (int i = 0; i < data.size(); i++)
			{
				// Serial.println(String(data[i]["setup"]));
				// Serial.println(String(data[i]["punchline"]));
				// Serial.println();

				stored_jokes.push_back(JOKE(data[i]["setup"], data[i]["punchline"]));
			}

			Serial.printf("Addded jokes, you now have %d\n\n", stored_jokes.size());
		}
		else
		{
			ok = false;
		}
	}
	catch (json::exception &e)
	{
		Serial.printf("response: %s\n", response);
		Serial.println("OW Json parse error:");
		Serial.println(e.what());
		next_update = millis();

		ok = false;
	}

	if (ok)
	{
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

	is_dirty = ok;
}

bool widgetJokes::redraw(uint8_t fade_amount, int8_t tab_group)
{
	bool was_dirty = false;

	if (process_next_joke)
	{
		process_next_joke = false;

		show_next_joke();
		return false;
	}
	// // we want the poll_frequency to be (mins in millis, so mins * 60 * 1000)
	// if ((millis() - next_update > 20000 && stored_jokes.size() == 0) || next_update == 0)
	// {
	// we only run this once
	if (!is_setup)
	{
		// next_update += 2000;

		is_setup = true;
		get_char_width();
		_sprite_joke.createVirtual(_w, _h, NULL, true);

		// Serial.println("Post is_setup jokes");

		if (settings.has_wifi_creds() && !server_path.empty() && !wifi_controller.wifi_blocking_access)
		{
			wifi_controller.add_to_queue(server_path, [this](bool success, const String &response) { this->process_joke_data(success, response); });
		}
	}
	// }

	if (is_busy)
	{
		// Serial.println("sis bizzzzie");
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{

		squixl.current_screen()->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		squixl.current_screen()->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		// Serial.println("Post readImage jokes");
		delay(10);

		draw_window_heading();

		// Serial.printf("\n***Hard Redraw JOKE Window with %d jokes loaded, fade: %d\n\n", stored_jokes.size(), fade_amount);

		_sprite_joke.fillScreen(TFT_MAGENTA);

		is_dirty_hard = false;
		is_aniamted_cached = false;
		was_dirty = true;
	}

	if (!is_aniamted_cached)
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
		squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_joke, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);
		squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

		// Serial.println("Tick jokes");
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

void widgetJokes::get_char_width()
{
	int16_t tempx;
	int16_t tempy;
	uint16_t tempw;
	uint16_t temph;

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.getTextBounds("W", 0, 0, &tempx, &tempy, &tempw, &temph);

	char_width = tempw;
	char_height = temph;
	max_chars_per_line = int((_w - 20) / char_width); // includes padding for margins
	max_lines = int((_h - 60) / char_height);		  // includes padding for margins ahd top heading

	// Serial.printf("char width: %d, max_chars_per_line: %d\n", char_width, max_chars_per_line);
}

void widgetJokes::process_lines()
{
	// Serial.println("Processing lines");
	lines.clear();
	wrap_text(stored_jokes[0].setup, max_chars_per_line);
	lines.push_back("*nl*");
	wrap_text(stored_jokes[0].punchline, max_chars_per_line);
	// this is basically clearig the joke sprite
	_sprite_joke.fillScreen(TFT_MAGENTA);
	is_aniamted_cached = false;
}

void widgetJokes::wrap_text(const String &text, int max_chars_per_line)
{

	String currentLine = "";
	int pos = 0;

	while (pos < text.length())
	{
		// Find the next space starting from pos.
		int spaceIndex = text.indexOf(' ', pos);
		String word;

		// If no more spaces, grab the rest of the string.
		if (spaceIndex == -1)
		{
			word = text.substring(pos);
			pos = text.length();
		}
		else
		{
			word = text.substring(pos, spaceIndex);
			pos = spaceIndex + 1; // Move past the space.
		}

		// If currentLine is empty, start it with the word.
		if (currentLine.length() == 0)
		{
			currentLine = word;
		}
		// Otherwise, check if adding the next word (with a space) would exceed the limit.
		else if (currentLine.length() + 1 + word.length() <= max_chars_per_line)
		{
			currentLine += " " + word;
		}
		// If it would exceed the limit, push the current line and start a new one.
		else
		{
			currentLine.replace("\n", " ");
			currentLine.replace("\r", " ");
			lines.push_back(currentLine);
			currentLine = word;
		}
	}

	// Add the last line if not empty.
	if (currentLine.length() > 0)
	{
		currentLine.replace("\n", " ");
		currentLine.replace("\r", " ");
		lines.push_back(currentLine);
	}
}

widgetJokes widget_jokes;
