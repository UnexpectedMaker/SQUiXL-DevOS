#ifndef SETTINGS_ASYNC_H
#define SETTINGS_ASYNC_H

#include <esp_heap_caps.h>
#include <memory>

template <typename T>
struct psram_allocator
{
		typedef T value_type;
		psram_allocator() = default;
		template <class U>
		constexpr psram_allocator(const psram_allocator<U> &) noexcept {}
		T *allocate(std::size_t n)
		{
			T *p = static_cast<T *>(heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
			if (!p)
				throw std::bad_alloc();
			return p;
		}
		void deallocate(T *p, std::size_t) noexcept
		{
			heap_caps_free(p);
		}
};
template <class T, class U>
bool operator==(const psram_allocator<T> &, const psram_allocator<U> &) { return true; }
template <class T, class U>
bool operator!=(const psram_allocator<T> &, const psram_allocator<U> &) { return false; }

#include <vector>
#include <map>
#include <string>
#include "utils/json_psram.h"
#include "utils/json_conversions.h"
#include "settings/settingsOption.h"
#include <functional>

// ==== ASYNC SUPPORT ====
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
// =======================

using json = nlohmann::json;

struct Config_locaton
{
		String country = "";
		String city = "";
		String state = "";
		String lon = "0";
		String lat = "0";
		int utc_offset = 999;
};

struct Config_screenshot
{
		bool enabled = true;
		float temperature = -0.1;
		float tint = 0.1;
		float black = 0.0;
		float white = 1.0;
		float gamma = 0.9;
		float saturation = 1.0;
		float contrast = 1.0;
};

struct mqtt_topic
{
		String name = "";
		String topic_listen = "";
		String topic_publish = "";

		// mqtt_topic(String _name, String _listen, String _publish) : name(_name), topic_listen(_listen), topic_publish(_publish) {};

		bool match_or_partial(String _listen, String _publish)
		{
			return (topic_listen == _listen || topic_publish == _publish);
		}
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

struct Config_audio
{
		bool ui = true;
		bool alarm = true;
		bool on_hour = false;
		bool charge = true;
		int current_radio_station = 0;
};

struct wifi_station
{
		std::string ssid = "";
		std::string pass = "";
		uint8_t channel = 9;
};

struct Config_widget_battery
{
		float perc_offset = 7.0;
		int low_perc = 25;
		float low_volt_warn = 3.5;
		float low_volt_cutoff = 3.2;
};

struct Config_expansion
{
		// false is 0x77 (secondary), true is 0x76 (primary)
		bool bme280_address = false;   // use primary or secondary I2C address for bme280
		bool bme280_installed = false; // Enable init, or not for the BME280
};

struct Config_widget_open_weather
{
		int poll_frequency = 30;
		String api_key = "";
		bool enabled = true;
		bool units_metric = true;

		bool has_key()
		{
			return (api_key.length() > 1);
		}
};

struct Config_widget_rss_feed
{
		int poll_frequency = 60;
		String feed_url = "https://rss.slashdot.org/slashdot/slashdotmain";
		bool enabled = true;

		bool has_url()
		{
			return (feed_url.length() > 1);
		}

		psram_string get_url()
		{
			return (psram_string)feed_url.c_str();
		}
};

struct Config
{
		int ver = 1;
		bool first_time = true;
		int screen_dim_mins = 10;

		int current_screen = 0;

		bool user_wallpaper = false;

		bool autostart_webserver = false;

		uint16_t case_color = 6371;

		bool ota_start = false;
		int wifi_tx_power = 44;
		bool use_local_dns = false;
		bool show_extra_wifi_details = false;
		bool wifi_check_for_updates = true;
		String mdns_name = "SQUiXL";

		std::vector<wifi_station> wifi_options;
		uint8_t current_wifi_station = 0;

		String ntp_server = "pool.ntp.org";

		// DEPRECIATED
		// Uses Confing_location
		String country = "";
		String city = "";
		String state = "";
		String lon = "0";
		String lat = "0";
		// DEPRECIATED

		int utc_offset = 999;

		bool time_24hour = false;
		bool time_dateformat = false;

		float volume = 15.0;

		int current_background = 0;
		int backlight_time_step_battery = 15;
		int backlight_time_step_vbus = 30;
		bool sleep_vbus = false;
		bool sleep_battery = true;

		Config_locaton location;
		Config_widget_battery battery;
		Config_widget_open_weather open_weather;
		Config_widget_rss_feed rss_feed;
		Config_audio audio;
		Config_mqtt mqtt;
		Config_haptics haptics;
		Config_screenshot screenshot;
		Config_expansion expansion;
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
	SCREENIE,
};

struct setting_group
{
		String name = "";
		String description = "";
		std::vector<SettingsOptionBase *, psram_allocator<SettingsOptionBase *>> groups = {};
		SettingType type = SettingType::MAIN;

		setting_group(String nm, SettingType t, String d = "") : name(nm), type(t), description(d) {};
};

class Settings
{

		struct BufferSaveReq
		{
				std::string path;
				const uint8_t *buffer;
				size_t length;
				std::function<void(bool ok)> cb;
		};

		struct LoadBufferReq
		{
				std::string path;
				std::function<void(bool, uint8_t *, size_t)> cb;
		};

	public:
		// std::vector<setting_group> settings_groups;
		std::vector<setting_group, psram_allocator<setting_group>> settings_groups;
		Config config;

		Settings(void)
		{
			// // Setup settings groups
			settings_groups.push_back({"General Settings", SettingType::MAIN});

			settings_groups.push_back({"WiFi & Web Settings", SettingType::WEB});

			settings_groups.push_back({"Audio Settings", SettingType::MAIN});

			settings_groups.push_back({"Haptics Settings", SettingType::MAIN});

			settings_groups.push_back({"Open Weather Settings", SettingType::WIDGET, "Add your Open Weather API key here to be able to see your current weather details on your SQUiXL."});

			settings_groups.push_back({"MQTT Settings", SettingType::WEB});

			settings_groups.push_back({"Screenie", SettingType::SCREENIE});

			settings_groups.push_back({"RSS Feed Settings", SettingType::WIDGET, "Add your RSS Feed URL here to be able to see your favourte RSS feed on your SQUiXL."});

			settings_groups.push_back({"Location Settings", SettingType::WEB});

			settings_groups.push_back({"Expansion Settings", SettingType::WIDGET, "I2C Expansion Port Settings"});
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
		bool ui_forced_save = false;

		String color565ToWebHex(uint16_t color565);
		uint16_t webHexToColor565(const char *hex);

		void save_buffer_async(const char *path, const uint8_t *buffer, size_t length, std::function<void(bool ok)> cb);
		void load_buffer_async(const char *path, std::function<void(bool ok, uint8_t *buffer, size_t length)> cb);

		SettingsOptionBool setting_time_24hour{&config.time_24hour, 0, "Time Mode", "12H", "24H"};
		SettingsOptionBool setting_time_dateformat{&config.time_dateformat, 0, "Date FMT", "DMY", "MDY"};
		SettingsOptionBool setting_wallpaper{&config.user_wallpaper, 0, "Wallpaper", "SYSTEM", "USER"};

		SettingsOptionColor565 setting_case_color{&config.case_color, 0, "Case Color"};
		SettingsOptionIntRange settings_backlight_timer_battery{&config.backlight_time_step_battery, 5, 30, 1, false, 0, "Battery Backlight Dimmer Time (secs)"};
		SettingsOptionIntRange settings_backlight_timer_vbus{&config.backlight_time_step_vbus, 5, 60, 1, false, 0, "5V Backlight Dimmer Time (secs)"};
		SettingsOptionBool setting_sleep_vbus{&config.sleep_vbus, 0, "Sleep On 5V", "NO", "YES"};
		SettingsOptionBool setting_sleep_battery{&config.sleep_battery, 0, "Sleep On Battery", "NO", "YES"};

		SettingsOptionBool setting_OTA_start{&config.ota_start, 1, "Enable OTA Updates", "NO", "YES"};
		SettingsOptionBool setting_wifi_check_updates{&config.wifi_check_for_updates, 1, "Notify Updates", "NO", "YES"};
		SettingsOptionString setting_web_mdns{&config.mdns_name, 1, "mDNS Name", 0, -1, "SQUiXL", false};

		SettingsOptionWiFiStations wifi_stations{&config.wifi_options, 1, "Wifi Stations"};
		SettingsOptionString setting_ntpserver{&config.ntp_server, 1, "NTP Server"};
		SettingsOptionBool setting_wifi_extra_details{&config.show_extra_wifi_details, 1, "Verbose WiFi Details", "NO", "YES"};
		SettingsOptionBool setting_wifi_local_dns{&config.use_local_dns, 1, "Use Local DNS Servers", "NO", "YES"};

		SettingsOptionBool setting_audio_ui{&config.audio.ui, 2, "UI Sound", "NO", "YES"};
		SettingsOptionBool setting_audio_alarm{&config.audio.alarm, 2, "Alarm Sound", "NO", "YES"};
		SettingsOptionBool setting_audio_on_hour{&config.audio.on_hour, 2, "Beep Hour", "NO", "YES"};
		SettingsOptionBool setting_audio_charge{&config.audio.charge, 2, "Start Charge", "NO", "YES"};
		SettingsOptionFloatRange setting_audio_volume{&config.volume, 0, 21, 1, false, 2, "Volume"};

		SettingsOptionBool setting_haptics_enabled{&config.haptics.enabled, 3, "Enabled", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_boot{&config.haptics.trigger_on_boot, 3, "On Boot", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_wake{&config.haptics.trigger_on_wake, 3, "On Wake", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_alarm{&config.haptics.trigger_on_alarm, 3, "On Alarm", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_hour{&config.haptics.trigger_on_hour, 3, "On Hour", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_event{&config.haptics.trigger_on_event, 3, "On Event", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_longpress{&config.haptics.trigger_on_longpress, 3, "LongPress", "NO", "YES"};
		SettingsOptionBool setting_haptics_trig_charge{&config.haptics.trigger_on_charge, 3, "Start Charge", "NO", "YES"};

		SettingsOptionBool widget_ow_enabled{&config.open_weather.enabled, 4, "Enabled", "NO", "YES"};
		SettingsOptionString widget_ow_apikey{&config.open_weather.api_key, 4, "API KEY", 0, -1, "", false};
		SettingsOptionIntRange widget_ow_poll_interval{&config.open_weather.poll_frequency, 10, 300, 10, false, 4, "OW Poll Interval (Min)"};
		SettingsOptionBool widget_ow_units{&config.open_weather.units_metric, 4, "Temperature Units", "Fahrenheit", "Celsius"};

		SettingsOptionBool mqtt_enabled{&config.mqtt.enabled, 5, "Enabled", "NO", "YES"};
		SettingsOptionString mqtt_broker_ip{&config.mqtt.broker_ip, 5, "Broker IP"};
		SettingsOptionInt mqtt_broker_port{&config.mqtt.broker_port, 5, 2000, false, 5, "Broker Port"};
		SettingsOptionString mqtt_username{&config.mqtt.username, 5, "Username", 0, -1, "", false};
		SettingsOptionString mqtt_password{&config.mqtt.password, 5, "Password", 0, -1, "", false};
		SettingsOptionString mqtt_device_name{&config.mqtt.device_name, 5, "Device Name"};
		SettingsOptionMQTTTopic mqtt_topics{&config.mqtt.topics, 5, "Topics"};
		// SettingsOptionString mqtt_topic_listen{&config.mqtt.topic_listen, 5, "Listen Topic"};

		SettingsOptionBool screenshot_enabled{&config.screenshot.enabled, 6, "Enabled", "NO", "YES"};
		SettingsOptionFloatRange screenshot_wb_temp{&config.screenshot.temperature, -1.0f, 1.0f, 0.1f, false, 6, "WB White Balance - Temperature"};
		SettingsOptionFloatRange screenshot_wb_tint{&config.screenshot.tint, -1.0f, 1.0f, 0.1f, false, 6, "White Balance -  Tint"};
		SettingsOptionFloatRange screenshot_black{&config.screenshot.black, 0.0f, 1.0f, 0.1f, false, 6, "Levels - Black"};
		SettingsOptionFloatRange screenshot_white{&config.screenshot.white, 0.0f, 1.0f, 0.1f, false, 6, "Levels - White"};
		SettingsOptionFloatRange screenshot_gamma{&config.screenshot.gamma, 0.0f, 2.0f, 0.1f, false, 6, "Levels - Gamma"};
		SettingsOptionFloatRange screenshot_saturation{&config.screenshot.saturation, 0.0f, 2.0f, 0.1f, false, 6, "Saturation"};
		SettingsOptionFloatRange screenshot_contrast{&config.screenshot.contrast, 0.0f, 2.0f, 0.1f, false, 6, "Contrast"};

		SettingsOptionBool widget_rss_enabled{&config.rss_feed.enabled, 7, "Enabled", "NO", "YES"};
		SettingsOptionString widget_rss_feed_url{&config.rss_feed.feed_url, 7, "Feed URL", 0, -1, "", false};
		SettingsOptionIntRange widget_rss_poll_interval{&config.rss_feed.poll_frequency, 10, 300, 60, false, 7, "RSS Poll Interval (Min)"};

		// Location
		SettingsOptionString setting_loc_country{&config.location.country, 8, "Country Code", 0, 2};
		SettingsOptionString setting_loc_city{&config.location.city, 8, "City"};
		SettingsOptionString setting_loc_state{&config.location.state, 8, "State"};
		SettingsOptionString setting_loc_lat{&config.location.lat, 8, "Latitude"};
		SettingsOptionString setting_loc_lon{&config.location.lon, 8, "Longitude"};
		SettingsOptionIntRange settings_utc_offset{&config.location.utc_offset, -12, 14, 1, false, 8, "UTC Offset"};

		// Expansion
		SettingsOptionBool expansion_bme_address{&config.expansion.bme280_address, 9, "I2C Address", "0x77", "0x76"};
		SettingsOptionBool expansion_bme_installed{&config.expansion.bme280_installed, 9, "Connected", "NO", "YES"};

		// ==== ASYNC SUPPORT ====
	public:
		volatile bool busy = false;

	private:
		enum AsyncOp
		{
			NONE = 0,
			LOAD,
			SAVE_FORCE,
			SAVE_NONFORCE,
			SAVE_BUFFER,
			LOAD_BUFFER
		};
		struct AsyncReq
		{
				AsyncOp op;
				bool force;
				BufferSaveReq *buffer_save = nullptr;
				LoadBufferReq *load_buffer_req = nullptr;
		};

		static void async_task(void *pv);
		static QueueHandle_t queue;
		static TaskHandle_t task_handle;
		void _start_async_task();
		void _schedule_async(AsyncOp op, bool force);
		bool _load_sync();
		bool _save_sync(bool force);

		static void async_buffer_save_task(void *pv); // Helper for large buffer saves if you want a dedicated task (optional)
													  // =======================

	private:
		static constexpr const char *filename = "/settings.json";
		static constexpr const char *tmp_filename = "/tmp_settings.json";
		static constexpr const char *log = "/log.txt";
		static constexpr const char *backup_prefix = "settings_back_";
		static const int max_backups = 10;
		static long backupNumber(const String);

		unsigned long max_time_between_saves = 10000;
		unsigned long last_save_time = 0;
};

extern Settings settings;

#endif // SETTINGS_H