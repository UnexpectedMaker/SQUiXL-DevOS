#ifndef SETTINGS_H
#define SETTINGS_H

#include <vector>
#include <map>
#include "utils/json.h"
#include "utils/json_conversions.h"
#include "settings/settingsOption.h"

using json = nlohmann::json;

struct mqtt_topic
{
		String name = "";
		String topic_listen = "";
		String topic_publish = "";
};

struct Config_mqtt
{
		bool enabled = false;
		String broker_ip = "";
		int broker_port = 1883;
		int keep_alive = 60;
		String username = "";
		String password = "";

		int retry_attemps = 5;

		String device_name = "squixl";

		std::vector<mqtt_topic> topics;

		String topic_listen = "squixl";
		String publish_topic = "squixl";

		bool has_ip()
		{
			return broker_ip.length() > 2;
		}
};

/**
 * @brief Settings struct for Haptics support
 */
struct Config_haptics
{
		bool enabled = true;
		bool trigger_on_boot = true;
		bool trigger_on_alarm = true;
		bool trigger_on_hour = true;
		bool trigger_on_event = false;
		bool trigger_on_wake = false;
		bool trigger_on_longpress = true;
		bool trigger_on_charge = true;
};

/**
 * @brief Settings struct for Haptics support
 */
struct Config_audio
{
		// Sound
		bool ui = true;
		bool alarm = true;
		bool on_hour = false;
		bool charge = true;
		int current_radio_station = 0;
};

struct wifi_station
{
		String ssid = "";
		String pass = "";
		uint8_t channel = 9;
};

struct Config_widget_battery
{
		// Fuel Gauge/Battery
		// This can be used to adjust what 100% is on your battery
		// This can be needed because the PMIC will stop charging the battery before it gets to 4.2V, usually around 4.1V,
		// So the fuel gauge will never actually get to a 100% state.
		// I'm not sure how to solve this othe than allow users to set an offset that visually shows full/no charging at 100%
		float perc_offset = 7.0;
		// User settable % to trigger fuel gauge wake from sleep trigger.
		// This can only be between 1% and 32%
		int low_perc = 25;
		// User settable V to trigger fuel gauge wake from sleep trigger.
		float low_volt_warn = 3.5;
		// User settable V to trigger power cutoff to the watch power switch.
		float low_volt_cutoff = 3.2;
};

/**
 * @brief Settings struct for open weather widget
 */
struct Config_widget_open_weather
{
		int poll_frequency = 30; // Open Weather poll interval - 30mins.
		String api_key = "";	 // API key for Open Weather
		bool enabled = true;
		bool units_metric = true;

		bool has_key()
		{
			return (api_key.length() > 1);
		}
};

// Save data struct
struct Config
{
		int ver = 1;
		bool first_time = true;
		int screen_dim_mins = 10;

		int current_screen = 0;

		bool beep_buttons = true;
		bool beep_activity = true;

		bool autostart_webserver = false;

		uint16_t case_color = 6371;

		bool ota_start = false;
		int wifi_tx_power = 44;

		bool wifi_check_for_updates = true;
		String mdns_name = "SQUiXL";

		std::vector<wifi_station> wifi_options;
		uint8_t current_wifi_station = 0;

		String country = "";
		String city = "";
		int utc_offset = 999;

		// Time
		bool time_24hour = false;
		bool time_dateformat = false; // False - DMY, True - MDY

		float volume = 15.0;

		int current_background = 0;

		// Battery/FG specific settings - see Struct above
		Config_widget_battery battery;

		// Open Weather specific settings - see Struct above
		Config_widget_open_weather open_weather;

		// Audio specific settings
		Config_audio audio;

		// MQTT specific settings - see Struct above
		Config_mqtt mqtt;

		// Haptics specific settings
		Config_haptics haptics;

		json last_saved_data;

		String case_color_in_hex()
		{
			uint16_t color565 = (uint16_t)case_color;

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
};

enum SettingType
{
	MAIN,
	WEB,
	WIDGET,
	THEME,
};

struct setting_group
{
		String name = "";
		String description = "";
		std::vector<SettingsOptionBase *> groups = {};
		SettingType type = SettingType::MAIN;

		setting_group(String nm, SettingType t, String d = "") : name(nm), type(t), description(d) {};

		// void setup(String nm, SettingType t, String d = "")
		// {
		// 	name = nm;
		// 	type = t;
		// 	description = d;
		// };
};

class Settings
{

	public:
		std::vector<setting_group> settings_groups;
		Config config;

		Settings(void)
		{
			// // Setup settings groups
			settings_groups.push_back({"General Settings", SettingType::MAIN});

			settings_groups.push_back({"WiFi & Web Settings", SettingType::WEB});

			settings_groups.push_back({"Audio Settings", SettingType::MAIN});

			settings_groups.push_back({"Haptics Settings", SettingType::MAIN});

			settings_groups.push_back({"Open Weather Settings", SettingType::WIDGET, "Add your Open Weather API key here to be able to see your current weather details on your watch face."});

			settings_groups.push_back({"MQTT Settings", SettingType::WEB});

			// settings_groups[7].setup("Watch Themes", SettingType::THEME);
		}

		void init();
		bool load();
		bool save(bool force);
		bool backup();
		bool create();
		void print_file();
		bool has_wifi_creds(void);
		bool has_country_set(void);
		void update_wifi_credentials(String ssid, String pass);
		unsigned long reset_screen_dim_time(void);
		bool update_menu = false;
		bool ui_forced_save = false; //

		String color565ToWebHex(uint16_t color565);
		uint16_t webHexToColor565(const char *hex);

		// Add any SettingsOption values here for any settings you want to bind with a tw_Control
		SettingsOptionBool setting_time_24hour{&config.time_24hour, 0, "Time Mode", "12H", "24H"};
		SettingsOptionBool setting_time_dateformat{&config.time_dateformat, 0, "Date FMT", "DMY", "MDY"};
		SettingsOptionColor565 setting_case_color{&config.case_color, 0, "Case Color"};

		// Web and WiFi
		SettingsOptionBool setting_OTA_start{&config.ota_start, 1, "Enable OTA Updates", "NO", "YES"};
		SettingsOptionBool setting_wifi_check_updates{&config.wifi_check_for_updates, 1, "Notify Updates", "NO", "YES"};
		SettingsOptionString setting_web_mdns{&config.mdns_name, 1, "mDNS Name", 0, -1, "SQUiXL", false};
		SettingsOptionString setting_country{&config.country, 1, "Country Code", 0, 2};
		SettingsOptionString setting_city{&config.city, 1, "City"};
		SettingsOptionIntRange settings_utc_offset{&config.utc_offset, -12, 14, 1, false, 1, "UTC Offset"};
		SettingsOptionWiFiStations wifi_stations{&config.wifi_options, 1, "Wifi Stations"};

		// Audio
		SettingsOptionBool setting_audio_ui{&config.audio.ui, 2, "UI Sound", "NO", "YES"};
		SettingsOptionBool setting_audio_alarm{&config.audio.alarm, 2, "Alarm Sound", "NO", "YES"};
		SettingsOptionBool setting_audio_on_hour{&config.audio.on_hour, 2, "Beep Hour", "NO", "YES"};
		SettingsOptionBool setting_audio_charge{&config.audio.charge, 2, "Start Charge", "NO", "YES"};
		SettingsOptionFloatRange setting_audio_volume{&config.volume, 0, 21, 1, false, 2, "Volume"};

		// haptics
		SettingsOptionBool setting_haptics_enabled{&config.haptics.enabled, 3, "Enabled", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_boot{&config.haptics.trigger_on_boot, 3, "On Boot", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_wake{&config.haptics.trigger_on_wake, 3, "On Wake", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_alarm{&config.haptics.trigger_on_alarm, 3, "On Alarm", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_hour{&config.haptics.trigger_on_hour, 3, "On Hour", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_event{&config.haptics.trigger_on_event, 3, "On Event", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_longpress{&config.haptics.trigger_on_longpress, 3, "LongPress", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_charge{&config.haptics.trigger_on_charge, 3, "Start Charge", "NO", "YES"};

		// Open Weather
		SettingsOptionBool widget_ow_enabled{&config.open_weather.enabled, 4, "Enabled", "NO", "YES"};
		SettingsOptionString widget_ow_apikey{&config.open_weather.api_key, 4, "API KEY", 0, -1, "", false};
		SettingsOptionIntRange widget_ow_poll_interval{&config.open_weather.poll_frequency, 10, 300, 10, false, 4, "Poll Interval (Min)"};
		SettingsOptionBool widget_ow_units{&config.open_weather.units_metric, 4, "Temperature Units", "Fahrenheit", "Celsius"};

		// MQTT
		SettingsOptionBool mqtt_enabled{&config.mqtt.enabled, 5, "Enabled", "NO", "YES"};
		SettingsOptionString mqtt_broker_ip{&config.mqtt.broker_ip, 5, "Broker IP"};
		SettingsOptionInt mqtt_broker_port{&config.mqtt.broker_port, 1, 2000, false, 5, "Broker Port"};
		SettingsOptionString mqtt_username{&config.mqtt.username, 5, "Username", 0, -1, "", false};
		SettingsOptionString mqtt_password{&config.mqtt.password, 5, "Password", 0, -1, "", false};
		SettingsOptionString mqtt_device_name{&config.mqtt.device_name, 5, "Device Name"};
		SettingsOptionString mqtt_topic_listen{&config.mqtt.topic_listen, 5, "Listen Topic"};

	private:
		static constexpr const char *filename = "/settings.json";
		static constexpr const char *tmp_filename = "/tmp_settings.json";
		static constexpr const char *log = "/log.txt";
		static constexpr const char *backup_prefix = "settings_back_";
		static const int max_backups = 10;
		static long backupNumber(const String);

		unsigned long max_time_between_saves = 10000; // every 10 seconds
		unsigned long last_save_time = 0;
};

extern Settings settings;

#endif // SETTINGS_H
