#include "ui/themes/themes.h"

#include <LittleFS.h>

using json = nlohmann::json;

/**
 * @brief Construct a new nlohmann define type non intrusive with default object
 *
 * All settings you want to be serialised and deserialised with JSON and stored in user flash need to be added here.
 * This has a HARD (NOT CHANGEABLE) LIMIT of 64 items
 */

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(theme, name, col_window_back, col_window_title_text, col_content_primary, col_content_secondary, col_content_highlight, col_warning, col_error, t_window_back, t_window_title);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(UIThemes, themes, current_theme);

theme &Themes::current()
{
	if (ui_themes.themes_list.themes.size() == 0 || themes_list.current_theme >= ui_themes.themes_list.themes.size())
		return default_theme;

	return ui_themes.themes_list.themes[themes_list.current_theme];
}

/**
 * @brief Load the theme settings from the FLASH FS and deserialise them from JSON back into the Config struct
 *
 * @return true
 * @return false
 */
bool Themes::load()
{
	Serial.println("Loading themes");

	File file = LittleFS.open(filename);
	if (!file || file.isDirectory() || file.size() == 0)
	{
		// No data on the flash chip, so create new data
		file.close();

		Serial.println("Themes CREATE: Creating new data...");

		ui_themes = {};

		save();

		return true;
	}

	std::vector<char> _data(file.size());
	size_t data_bytes_read = file.readBytes(_data.data(), _data.size());
	if (data_bytes_read != _data.size())
	{
		// Reading failed
		String log = "bad read " + String(file.size()) + " " + String((int)data_bytes_read);
		// log_to_nvs("load_status", log.c_str());
		file.close();
		// create();
		return false;
	}

	try
	{
		json json_data = json::parse(_data);

		// Convert json to struct
		themes_list = json_data.get<UIThemes>();
	}
	catch (json::exception &e)
	{
		Serial.println("Settings parse error:");
		Serial.println(e.what());
		file.close();
		// create();
		// log_to_nvs("load_status", "bad json parse");
		return false;
	}

	file.close();

	if (ui_themes.themes_list.themes.size() == 0)
	{
		theme example;
		example.name = "Example Theme";

		ui_themes.themes_list.themes.push_back(example);
	}

	return true;
}

bool Themes::save()
{
	// // We only want to attempt  save every 1 min unless it's a forced save.
	// if (!force && millis() - last_save_time < max_time_between_saves)
	// 	return false;

	// Implicitly convert struct to json
	json data = themes_list;

	// // If the data is the same as the last data we saved, bail out
	if (data == themes_list.last_saved_data)
	{
		return false;
	}

	std::string serializedObject = data.dump();

	// Serial.print("Data Length: "+String(serializedObject.length())+"-> ");
	// Serial.println(serializedObject);

	File file = LittleFS.open(tmp_filename, FILE_WRITE);
	if (!file)
	{
		Serial.println("Failed to write to themes file");
		// log_to_nvs("save_status", "failed to open for write");
		return false;
	}

	file.print(serializedObject.c_str());
	// log_to_nvs("save_status", "data written");

	file.close();
	// log_to_nvs("save_status", "file closed");

	LittleFS.rename(tmp_filename, filename);
	// log_to_nvs("save_status", "file renamed");

	Serial.println("Themes SAVE: Saved!");

	// Store last saved data for comparison on next save
	themes_list.last_saved_data.swap(data);

	// last_save_time = millis();
	return true;
}

Themes ui_themes;