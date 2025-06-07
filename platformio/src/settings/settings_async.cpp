#include "Arduino.h"
#include "settings_async.h"
#include <LittleFS.h>
#include "bb_spi_lcd.h"

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(wifi_station, ssid, pass, channel);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_screen, inversion_mode);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_mqtt, enabled, broker_ip, broker_port, username, password, device_name, topic_listen, publish_topic);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_audio, ui, on_hour, charge, current_radio_station);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_screenshot, temperature, tint, gamma, saturation, contrast, black, white, enabled);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_haptics, enabled, trigger_on_boot, trigger_on_alarm, trigger_on_hour, trigger_on_event, trigger_on_wake, trigger_on_longpress, trigger_on_charge);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_widget_open_weather, enabled, api_key, poll_frequency, units_metric);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config_widget_rss_feed, enabled, feed_url, poll_frequency);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Config, first_time, current_screen, ota_start, wifi_tx_power, wifi_options, current_wifi_station, wifi_check_for_updates, mdns_name, case_color, ntp_server, city, country, utc_offset, time_24hour, time_dateformat, volume, current_background, backlight_time_step_battery, backlight_time_step_vbus, sleep_vbus, sleep_battery, open_weather, rss_feed, audio, mqtt, haptics, screenshot, user_wallpaper, screen);

static uint32_t min_clk_freq = 6000000;
static uint32_t max_clk_freq = 7000000;

// // === VSYNC SUPPORT ===
// extern volatile bool vsync_flag; // You must set this true in your panel VSYNC callback
// inline void wait_for_vsync()
// {
// 	while (!vsync_flag)
// 		vTaskDelay(1);
// 	vsync_flag = false;
// }
// =======================

// === ASYNC TASK INFRASTRUCTURE ===
QueueHandle_t Settings::queue = nullptr;
TaskHandle_t Settings::task_handle = nullptr;

void Settings::_start_async_task()
{
	if (!queue)
		queue = xQueueCreate(2, sizeof(AsyncReq));
	if (!task_handle)
	{
		xTaskCreatePinnedToCore(async_task, "settings_async", 4096 * 2, this, 2, &task_handle, 0);
	}
}

void Settings::_schedule_async(AsyncOp op, bool force)
{
	_start_async_task();
	AsyncReq req = {op, force};
	xQueueSend(queue, &req, portMAX_DELAY);
}

void Settings::async_task(void *pv)
{
	Settings *self = reinterpret_cast<Settings *>(pv);
	AsyncReq req;
	while (1)
	{
		if (xQueueReceive(self->queue, &req, portMAX_DELAY) == pdTRUE)
		{
			self->busy = true;
			if (req.op == LOAD)
				self->_load_sync();
			else if (req.op == SAVE_FORCE)
				self->_save_sync(true);
			else if (req.op == SAVE_NONFORCE)
				self->_save_sync(false);
			else if (req.op == SAVE_BUFFER && req.buffer_save)
			{
				// Call a helper that writes the buffer to FS, then calls the callback
				bool ok = false;

				// wait_for_vsync(); // <---- Added VSYNC WAIT
				// RGBChangeFreq(min_clk_freq);
				// wait_for_vsync(); // <---- Added VSYNC WAIT

				File file = LittleFS.open(req.buffer_save->path.c_str(), FILE_WRITE);
				if (file)
				{
					size_t written = file.write(req.buffer_save->buffer, req.buffer_save->length);
					file.close();
					ok = (written == req.buffer_save->length);
				}
				else
				{
					ok = false;
				}

				// Restore LCD freq as needed here

				// Call the callback on completion
				if (req.buffer_save->cb)
					req.buffer_save->cb(ok);

				// Free the buffer (the upload code expects ownership here)
				free((void *)req.buffer_save->buffer);
				delete req.buffer_save;

				// wait_for_vsync(); // <---- Added VSYNC WAIT
				// RGBChangeFreq(max_clk_freq);
				// wait_for_vsync(); // <---- Added VSYNC WAIT
			}
			else if (req.load_buffer_req)
			{
				// wait_for_vsync();
				// RGBChangeFreq(min_clk_freq);
				// wait_for_vsync();

				File file = LittleFS.open(req.load_buffer_req->path.c_str(), FILE_READ);
				bool ok = false;
				uint8_t *buf = nullptr;
				size_t sz = 0;
				if (file)
				{
					sz = file.size();
					if (sz > 0)
					{
						buf = (uint8_t *)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
						if (buf)
						{
							size_t actual = file.read(buf, sz);
							ok = (actual == sz);
						}
					}
					file.close();
				}

				req.load_buffer_req->cb(ok, buf, sz);
				if (buf)
					free(buf); // Caller must NOT free; we do it here after the callback!
				delete req.load_buffer_req;

				// wait_for_vsync();
				// RGBChangeFreq(max_clk_freq);
				// wait_for_vsync();
			}
			self->busy = false;
		}
	}
}

// -------------- ASYNC versions of load/save -------------------
bool Settings::load()
{
	_schedule_async(LOAD, false);
	return true;
}

bool Settings::save(bool force)
{

	if (!force && millis() - last_save_time < max_time_between_saves)
		return false;

	_schedule_async(force ? SAVE_FORCE : SAVE_NONFORCE, force);
	return true;
}

// -------------- Original sync code, made private, called by async task ---------------
bool Settings::_load_sync()
{
	// wait_for_vsync(); // <---- Added VSYNC WAIT
	// RGBChangeFreq(min_clk_freq);
	// wait_for_vsync(); // <---- Added VSYNC WAIT

	Serial.println("Loading settings");

	File file = LittleFS.open(filename);
	if (!file || file.isDirectory() || file.size() == 0)
	{
		file.close();
		create();
		return false;
	}

	std::vector<char> _data(file.size());
	size_t data_bytes_read = file.readBytes(_data.data(), _data.size());
	if (data_bytes_read != _data.size())
	{
		file.close();
		create();
		return false;
	}

	try
	{
		json json_data = json::parse(_data);

		config = json_data.get<Config>();
		config.last_saved_data.swap(json_data);
	}
	catch (json::exception &e)
	{
		Serial.println("Settings parse error:");
		Serial.println(e.what());
		file.close();
		create();
		return false;
	}

	file.close();

	bool user_wallpaper_exists = LittleFS.exists("/user_wallpaper.jpg");
	Serial.printf("** LOAD: Wallpaper stuff: user_wallpaper_exists ? %d, config.user_wallpaper? %d\n\n", user_wallpaper_exists, config.user_wallpaper);
	if (config.user_wallpaper && !user_wallpaper_exists)
		config.user_wallpaper = false;

	config.current_wifi_station = 0;
	config.open_weather.poll_frequency = 5;
	if (config.city == "Sydney")
		config.city = "Melbourne";

	Serial.printf("Country: %s, utc offset is %u\n", config.country, config.utc_offset);

	if (config.mqtt.broker_ip == "" || config.mqtt.broker_ip == "mqtt://192.168.1.70")
	{
		config.mqtt.broker_ip = "192.168.1.70";
	}

	Serial.println("Settings: Load complete!");

	// wait_for_vsync(); // <---- Added VSYNC WAIT
	// RGBChangeFreq(max_clk_freq);
	// wait_for_vsync(); // <---- Added VSYNC WAIT

	return true;
}

bool Settings::_save_sync(bool force)
{
	json data = config;

	if (!force && data == config.last_saved_data && !ui_forced_save)
	{
		last_save_time = millis();
		return false;
	}

	// wait_for_vsync(); // <---- Added VSYNC WAIT
	// RGBChangeFreq(min_clk_freq);
	// wait_for_vsync(); // <---- Added VSYNC WAIT

	ui_forced_save = false;

	std::string serializedObject = data.dump();

	File file = LittleFS.open(tmp_filename, FILE_WRITE);
	if (!file)
	{
		Serial.println("Failed to write to settings file");
		return false;
	}

	file.print(serializedObject.c_str());
	file.close();

	LittleFS.rename(tmp_filename, filename);

	Serial.println("Settings SAVE: Saved!");

	config.last_saved_data.swap(data);

	last_save_time = millis();

	// wait_for_vsync(); // <---- Added VSYNC WAIT
	// RGBChangeFreq(max_clk_freq);
	// wait_for_vsync(); // <---- Added VSYNC WAIT

	return true;
}

void Settings::save_buffer_async(const char *path, const uint8_t *buffer, size_t length, std::function<void(bool ok)> cb)
{
	_start_async_task();

	// Create a request object
	BufferSaveReq *req = new BufferSaveReq;
	req->path = path;
	req->length = length;
	// Copy buffer pointer (not copy of data, we own the buffer)
	req->buffer = buffer;
	req->cb = cb;

	AsyncReq areq;
	areq.op = SAVE_BUFFER;
	areq.buffer_save = req;
	areq.force = false; // not used

	// Enqueue
	xQueueSend(queue, &areq, portMAX_DELAY);
}

void Settings::load_buffer_async(const char *path, std::function<void(bool ok, uint8_t *buffer, size_t length)> cb)
{
	_start_async_task();
	LoadBufferReq *req = new LoadBufferReq{path, cb};
	AsyncReq areq = {LOAD_BUFFER, false, nullptr, req};
	xQueueSend(queue, &areq, portMAX_DELAY);
}
// ==================== Everything below is unchanged =====================

unsigned long Settings::reset_screen_dim_time()
{
	return (millis() + config.screen_dim_mins * 60 * 1000);
}

void Settings::init()
{
	setting_time_24hour.register_option();
	setting_time_dateformat.register_option();
	setting_case_color.register_option();
	settings_backlight_timer_battery.register_option();
	settings_backlight_timer_vbus.register_option();
	setting_sleep_vbus.register_option();
	setting_sleep_battery.register_option();
	setting_wallpaper.register_option();

	// Web and WiFi
	setting_OTA_start.register_option();
	setting_wifi_check_updates.register_option();
	setting_web_mdns.register_option();
	setting_country.register_option();
	setting_city.register_option();
	settings_utc_offset.register_option();
	wifi_stations.register_option();
	setting_ntpserver.register_option();
	setting_wifi_extra_details.register_option();

	// Open Weather
	widget_ow_enabled.register_option();
	widget_ow_apikey.register_option();
	widget_ow_poll_interval.register_option();
	widget_ow_units.register_option();

	// RSS Feed
	widget_rss_enabled.register_option();
	widget_rss_feed_url.register_option();
	widget_rss_poll_interval.register_option();

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

	// Audio UI
	setting_audio_ui.register_option();
	setting_audio_alarm.register_option();
	setting_audio_on_hour.register_option();
	setting_audio_charge.register_option();
	setting_audio_volume.register_option();

	// Snapshot Image Settings
	screenshot_saturation.register_option();
	screenshot_contrast.register_option();
	screenshot_black.register_option();
	screenshot_white.register_option();
	screenshot_gamma.register_option();

	screenshot_wb_temp.register_option();
	screenshot_wb_tint.register_option();
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
	if (*hex == '#')
		++hex;

	auto hv = [](char c) -> uint8_t {
		if (c >= '0' && c <= '9')
			return c - '0';
		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
		return 0;
	};

	uint8_t r8 = (hv(hex[0]) << 4) | hv(hex[1]);
	uint8_t g8 = (hv(hex[2]) << 4) | hv(hex[3]);
	uint8_t b8 = (hv(hex[4]) << 4) | hv(hex[5]);

	uint8_t r5 = (r8 * 31 + 127) / 255;
	uint8_t g6 = (g8 * 63 + 127) / 255;
	uint8_t b5 = (b8 * 31 + 127) / 255;

	return (r5 << 11) | (g6 << 5) | b5;
}

bool Settings::has_wifi_creds(void)
{
	return config.wifi_options.size() > 0 && config.wifi_options[0].ssid != "" && config.wifi_options[0].pass != "";
}

bool Settings::has_country_set(void) { return !config.country.isEmpty(); }

void Settings::update_wifi_credentials(String ssid, String pass)
{
	if (config.wifi_options.size() == 0)
	{
		wifi_station station = wifi_station();
		station.ssid = ssid.c_str();
		station.pass = pass.c_str();
		config.wifi_options.push_back(station);
	}
	else if (config.wifi_options.size() == 1)
	{
		config.wifi_options[0].ssid = ssid.c_str();
		config.wifi_options[0].pass = pass.c_str();
	}
	save(true);
}

long Settings::backupNumber(const String filename)
{
	if (!filename.startsWith(backup_prefix) || !filename.endsWith(".json"))
		return 0;
	return filename.substring(strlen(backup_prefix)).toInt();
}

bool Settings::backup()
{
	return true;
}

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