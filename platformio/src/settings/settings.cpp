#include "Arduino.h"
#include "settings.h"
#include <LittleFS.h>
#include "bb_spi_lcd.h"

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(wifi_station, ssid, pass, channel);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_mqtt, enabled, broker_ip, broker_port, username, password, device_name, topic_listen, publish_topic);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_audio, ui, on_hour, charge, current_radio_station);

// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_widget_battery, perc_offset, low_perc, low_volt_warn, low_volt_cutoff);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_haptics, enabled, trigger_on_boot, trigger_on_alarm, trigger_on_hour, trigger_on_event, trigger_on_wake, trigger_on_longpress, trigger_on_charge);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_widget_open_weather, enabled, api_key, poll_frequency, units_metric);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config, first_time, current_screen, ota_start, wifi_tx_power, wifi_options, current_wifi_station, wifi_check_for_updates, mdns_name, case_color, city, country, utc_offset, time_24hour, time_dateformat, volume, current_background);

unsigned long Settings::reset_screen_dim_time()
{
	return (millis() + config.screen_dim_mins * 60 * 1000);
}

void Settings::init()
{
	setting_time_24hour.register_option();
	setting_time_dateformat.register_option();
	setting_case_color.register_option();

	// Web and WiFi
	setting_OTA_start.register_option();
	setting_wifi_check_updates.register_option();
	setting_web_mdns.register_option();
	setting_country.register_option();
	setting_city.register_option();
	settings_utc_offset.register_option();
	wifi_stations.register_option();

	// Open Weather
	widget_ow_enabled.register_option();
	widget_ow_apikey.register_option();
	widget_ow_poll_interval.register_option();
	widget_ow_units.register_option();

	// Haptics
	setting_haptics_enabled.register_option();
	setting_haptics_trig_boot.register_option();
	setting_haptics_trig_wake.register_option();
	setting_haptics_trig_alarm.register_option();
	setting_haptics_trig_hour.register_option();
	setting_haptics_trig_event.register_option();
	setting_haptics_trig_longpress.register_option();
	setting_haptics_trig_charge.register_option();

	// MQTT
	mqtt_enabled.register_option();
	mqtt_broker_ip.register_option();
	mqtt_broker_port.register_option();
	mqtt_username.register_option();
	mqtt_password.register_option();
	mqtt_device_name.register_option();
	mqtt_topic_listen.register_option();

	setting_audio_ui.register_option();
	setting_audio_alarm.register_option();
	setting_audio_on_hour.register_option();
	setting_audio_charge.register_option();
}

String Settings::color565ToWebHex(uint16_t color565)
{
	uint8_t r5 = (color565 >> 11) & 0x1F;
	uint8_t g6 = (color565 >> 5) & 0x3F;
	uint8_t b5 = color565 & 0x1F;

	uint8_t r8 = (r5 << 3) | (r5 >> 2);
	uint8_t g8 = (g6 << 2) | (g6 >> 4);
	uint8_t b8 = (b5 << 3) | (b5 >> 2);

	char buf[8];
	snprintf(buf, sizeof(buf), "#%02X%02X%02X", r8, g8, b8);
	return String(buf);
}

uint16_t Settings::webHexToColor565(const char *hex)
{
	// skip leading '#'
	if (*hex == '#')
		++hex;

	// inline hex→nibble
	auto hv = [](char c) -> uint8_t {
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		return 0;
	};

	// parse bytes
	uint8_t r8 = (hv(hex[0]) << 4) | hv(hex[1]);
	uint8_t g8 = (hv(hex[2]) << 4) | hv(hex[3]);
	uint8_t b8 = (hv(hex[4]) << 4) | hv(hex[5]);

	// downscale with rounding
	uint8_t r5 = (r8 * 31 + 127) / 255;
	uint8_t g6 = (g8 * 63 + 127) / 255;
	uint8_t b5 = (b8 * 31 + 127) / 255;

	// pack
	return (r5 << 11) | (g6 << 5) | b5;
}

/**
 * @brief Checks to see if there are WiFi credentials stores in the user settings
 *
 * @return true credentials are not empty strings
 * @return false credentials are empty strings
 */
bool Settings::has_wifi_creds(void)
{
	return config.wifi_options.size() > 0 && !config.wifi_options[0].ssid.isEmpty() && !config.wifi_options[0].pass.isEmpty();
}

bool Settings::has_country_set(void) { return !config.country.isEmpty(); }

/**
 * @brief Update the users WiFi credentials in the settings struct
 *
 * @param ssid
 * @param pass
 */
void Settings::update_wifi_credentials(String ssid, String pass)
{
	if (config.wifi_options.size() == 0)
	{
		wifi_station station = wifi_station();
		station.ssid = ssid;
		station.pass = pass;
		config.wifi_options.push_back(station);
	}
	else if (config.wifi_options.size() == 1)
	{
		config.wifi_options[0].ssid = ssid;
		config.wifi_options[0].pass = pass;
	}
	// TODO:  Need to add checking of existing SSID/PASS bombo, and if not found add new entry
	// 	config.wifi_station
	// 		config.wifi_ssid = ssid;
	// config.wifi_pass = pass;
	save(true);
}

// String Settings::get_load_status()
// {
// 	String log = "";
// 	nvs.begin("flash_log");
// 	log = nvs.getString("load_status", "load_nada");
// 	nvs.end();

// 	return log;
// }

// String Settings::get_save_status()
// {
// 	String log = "";
// 	nvs.begin("flash_log");
// 	log = nvs.getString("save_status", "save_nada");
// 	nvs.end();

// 	return log;
// }

/**
 * @brief Load the user settings from the FLASH FS and deserialise them from JSON back into the Config struct
 *
 * @return true
 * @return false
 */
bool Settings::load()
{
	Serial.println("Loading settings");

	File file = LittleFS.open(filename);
	if (!file || file.isDirectory() || file.size() == 0)
	{
		// No data on the flash chip, so create new data
		file.close();
		create();
		// log_to_nvs("load_status", "no file");
		return false;
	}

	std::vector<char> _data(file.size());
	size_t data_bytes_read = file.readBytes(_data.data(), _data.size());
	if (data_bytes_read != _data.size())
	{
		// Reading failed
		String log = "bad read " + String(file.size()) + " " + String((int)data_bytes_read);
		// log_to_nvs("load_status", log.c_str());
		file.close();
		create();
		return false;
	}

	try
	{
		json json_data = json::parse(_data);

		// Convert json to struct
		config = json_data.get<Config>();

		// Store loaded data for comparison on next save
		config.last_saved_data.swap(json_data);
	}
	catch (json::exception &e)
	{
		Serial.println("Settings parse error:");
		Serial.println(e.what());
		file.close();
		create();
		// log_to_nvs("load_status", "bad json parse");
		return false;
	}

	file.close();

	config.current_wifi_station = 0;
	config.open_weather.poll_frequency = 5;
	if (config.city == "Sydney")
		config.city = "Melbourne";

	Serial.printf("Country: %s, utc offset is %u\n", config.country, config.utc_offset);

	if (config.mqtt.broker_ip = "" || config.mqtt.broker_ip == "mqtt://192.168.1.70")
	{
		config.mqtt.broker_ip = "192.168.1.70";
	}

	Serial.println("Settings: Load complete!");

	return true;
}

// Return the number from a string in the format "settings_back_N.json" for N>0 or 0 if wrong format
long Settings::backupNumber(const String filename)
{
	if (!filename.startsWith(backup_prefix) || !filename.endsWith(".json"))
		return 0;
	// toInt() returns zero if not a number
	return filename.substring(strlen(backup_prefix)).toInt();
}

// TODO: Reimplement settings backup
bool Settings::backup()
{
	// Find the backup file with the highest number
	long highestBackup = 0;
	// File rootDir = SD.open("/");
	// while (true)
	// {
	// 	File file = rootDir.openNextFile();
	// 	if (!file)
	// 		break;
	// 	highestBackup = max(highestBackup, backupNumber(file.name()));
	// 	file.close();
	// }

	// // Delete older files to satisfy max_backups
	// rootDir.rewindDirectory();
	// while (true)
	// {
	// 	File file = rootDir.openNextFile();
	// 	if (!file)
	// 		break;
	// 	long num = backupNumber(file.name());
	// 	if (num > 0 && num <= highestBackup + 1 - max_backups)
	// 	{
	// 		Serial.printf("Remove old backup %s\n", file.name());
	// 		// Removing an open file might be bad.
	// 		// We close it before removing, making sure to take a copy of the path as will be free'd
	// 		String path = file.path();
	// 		file.close();
	// 		SD.remove(path);
	// 	}
	// 	file.close();
	// }
	// rootDir.close();

	// SD.rename(filename, "/" + String(backup_prefix) + String(highestBackup + 1) + ".json");

	return true;
}

/**
 * @brief Serialise the Config struct into JSON and save to the FLASH FS
 * Only check for save every 5 mins, and then only save if the data has changed
 *
 * We only want to save data when it's changed because we dont want to wear out the Flash.
 *
 * @param force for the save regardless of time, but again, only if the data has changed
 * @return true
 * @return false
 */
bool Settings::save(bool force)
{
	// We only want to attempt  save every 1 min unless it's a forced save.
	if (!force && millis() - last_save_time < max_time_between_saves)
		return false;

	// Implicitly convert struct to json
	json data = config;

	// If the data is the same as the last data we saved, bail out
	if (!force && data == config.last_saved_data && !ui_forced_save)
	{
		last_save_time = millis();
		return false;
	}

	ui_forced_save = false;

	// Backup the settings file before we save new settings!
	// backup();

	// Serial.println(F("Settings SAVE: Saving data..."));

	std::string serializedObject = data.dump();

	// Serial.print("Data Length: "+String(serializedObject.length())+"-> ");
	// Serial.println(serializedObject);

	// SetRGB_ClockSpeed(600000);

	// RGB_Set_SPI_Speed(4000000);
	// delay(50);

	File file = LittleFS.open(tmp_filename, FILE_WRITE);
	if (!file)
	{
		Serial.println("Failed to write to settings file");
		// log_to_nvs("save_status", "failed to open for write");
		return false;
	}

	file.print(serializedObject.c_str());
	// log_to_nvs("save_status", "data written");

	file.close();
	// log_to_nvs("save_status", "file closed");

	LittleFS.rename(tmp_filename, filename);
	// log_to_nvs("save_status", "file renamed");

	Serial.println("Settings SAVE: Saved!");

	// RGB_Set_SPI_Speed(12000000);

	// Store last saved data for comparison on next save
	config.last_saved_data.swap(data);

	// SetRGB_ClockSpeed(1200000);

	last_save_time = millis();
	return true;
}

/**
 * @brief Create a new set of save data, either because this is the very first save, or because the load failed due to FS corruption,
 * or malformed JSON data that could not be deserialised.
 *
 * Once created, the data is automatically saved to flash.
 *
 * @return true
 * @return false
 */
bool Settings::create()
{
	Serial.println("Settings CREATE: Creating new data...");

	config = {};

	save(true);

	return true;
}

void Settings::print_file()
{
	File file = LittleFS.open(filename);
	std::vector<char> _data(file.size());
	size_t data_bytes_read = file.readBytes(_data.data(), _data.size());

	Serial.println("Settings JSON");
	for (char c : _data)
	{
		Serial.print(c);
	}
	Serial.println();

	file.close();
}

Settings settings;