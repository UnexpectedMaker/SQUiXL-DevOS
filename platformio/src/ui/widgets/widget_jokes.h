#pragma once

#include "ui/ui_window.h"

struct JOKE
{
		String setup;
		String punchline;

		JOKE(String s, String p) : setup(s), punchline(p) {}
};

class widgetJokes : public ui_window
{
	public:
		// void draw(uint canvasid);
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event);

		void process_joke_data(bool success, const String &response);

		void show_next_joke();

	private:
		std::string server_path = "https://official-joke-api.appspot.com/jokes/random/5";
		unsigned long next_update = 0;
		unsigned long next_joke_swap = 0;

		bool should_redraw = false;
		bool is_setup = false;
		bool process_next_joke = false;
		bool is_getting_more_jokes = false;

		uint8_t char_width = 0;
		uint8_t char_height = 0;
		uint8_t max_chars_per_line = 0;
		uint8_t max_lines = 0;

		std::vector<String> lines;

		std::vector<JOKE> stored_jokes;

		BB_SPI_LCD _sprite_joke;

		void process_lines();
		void wrap_text(const String &text, int max_chars_per_line);

		void reset_refresh_timer();
};

extern widgetJokes widget_jokes;