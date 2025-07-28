#pragma once

#include "Arduino.h"
#include "squixl.h"
#include "utils/json_psram.h"
#include "utils/json_conversions.h"
#include <vector>

using json = nlohmann::json;

struct theme
{
		String name;
		// Fonts Mono
		GFXfont *font_Mono_B = nullptr;
		GFXfont *font_Mono_R = nullptr;
		GFXfont *font_Mono_L = nullptr;
		// Fonts Propotional
		GFXfont *font_B = nullptr;
		GFXfont *font_R = nullptr;
		GFXfont *font_L = nullptr;

		// Colors
		uint16_t col_window_back = 0;
		uint16_t col_window_title_bar = 0;
		uint16_t col_window_title_text = 0;
		uint16_t col_content_primary = 0;
		uint16_t col_content_secondary = 0;
		uint16_t col_content_highlight = 0;
		uint16_t col_warning = 0;
		uint16_t col_error = 0;

		// Transparency
		uint8_t t_window_back = 12;
		uint8_t t_window_title = 12;
};

struct UIThemes
{
		std::vector<theme> themes;
		uint8_t current_theme = 0;
		json last_saved_data;
};

class Themes
{
	public:
		Themes()
		{
			default_theme.name = "Default Dark";
		};

		theme &current();
		UIThemes themes_list;

		bool load();
		bool save();
		void print_file(void);

	private:
		theme default_theme;
		static constexpr const char *filename = "/themes.json";
		static constexpr const char *tmp_filename = "/tmp_themes.json";
};

extern Themes ui_themes;