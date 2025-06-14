#include "squixl.h"

#include "ui/icons/images/ui_icons.h"
#include "ui/ui_screen.h"

#include "ui/widgets/widget_openweather.h"
#include "ui/widgets/widget_jokes.h"
#include "ui/widgets/widget_rss_feeds.h"
#include "ui/widgets/widget_time.h"
#include "ui/widgets/widget_bme280.h"
#include "ui/widgets/widget_battery.h"
#include "ui/widgets/widget_wifimanager.h"

#include "ui/controls/ui_control_button.h"
#include "ui/controls/ui_control_toggle.h"
#include "ui/controls/ui_control_slider.h"
#include "ui/controls/ui_control_textbox.h"
#include "ui/ui_label.h"
#include "ui/ui_scrollarea.h"

#include "ui/controls/ui_control_tabgroup.h"
#include "ui/ui_dialogbox.h"

#include "mqtt/mqtt.h"
#include "utils/littlefs_cli.h"

unsigned long next_background_swap = 0;
unsigned long every_second = 0;
unsigned long ntp_time_set = 0;

bool start_webserver = true;
unsigned long delay_webserver_start = 0;

bool ui_initialised = false;
bool was_asleep = false;

// UI stuff

ui_screen screen_wifi_setup;
ui_screen screen_main;
ui_screen screen_mqtt;
ui_screen screen_settings;

ui_screen screen_wifi_manager;

// Settings
ui_control_tabgroup settings_tab_group;
ui_control_slider slider_backlight_timer_battery;
ui_control_slider slider_backlight_timer_vbus;
ui_control_toggle toggle_sleep_vbus;
ui_control_toggle toggle_sleep_battery;
ui_control_toggle toggle_wallpaper;
// Time
ui_control_toggle toggle_time_mode;
ui_control_toggle toggle_date_mode;
ui_control_slider slider_UTC;
ui_control_textbox text_ntpserver;
// WiFi
ui_control_toggle toggle_OTA_updates;
ui_control_toggle toggle_Notify_updates;
ui_control_toggle toggle_verbose_wifi;
ui_control_toggle toggle_local_dns;
// Audio
ui_control_toggle toggle_audio_ui;
ui_control_toggle toggle_audio_alarm;
ui_control_slider slider_volume;
// Haptics
ui_control_toggle toggle_haptics_enable;
// Open Weather
ui_control_toggle toggle_ow_enable;
ui_control_slider slider_ow_refresh;
ui_control_textbox text_ow_api_key;
// Location
ui_control_textbox text_loc_country;
ui_control_textbox text_loc_city;
ui_control_textbox text_loc_state;
ui_control_textbox text_loc_lon;
ui_control_textbox text_loc_lat;
ui_control_button button_get_lon_lat;
// RSS Feed
ui_control_toggle toggle_rss_enable;
ui_control_slider slider_rss_refresh;
ui_control_textbox text_rss_feed_url;
// Expansion
ui_control_toggle toggle_bme280_I2C_address;
// MQTT
ui_control_toggle toggle_mqtt_enable;
ui_control_textbox text_mqtt_broker_ip;
ui_control_textbox text_mqtt_broker_port;
ui_control_textbox text_mqtt_broker_username;
ui_control_textbox text_mqtt_broker_password;

// // Screenshot stuff
// ui_control_toggle toggle_screenshot_enable;
// ui_control_slider slider_screenshot_wb_temp;
// ui_control_slider slider_screenshot_wb_tint;
// ui_control_slider slider_screenshot_lvl_black;
// ui_control_slider slider_screenshot_lvl_white;
// ui_control_slider slider_screenshot_lvl_gamma;
// ui_control_slider slider_screenshot_saturation;
// ui_control_slider slider_screenshot_contrast;

ui_control_button button_dialogbox_test;

ui_label label_version;
ui_scrollarea mqtt_notifications;

void button_press_ok()
{
	Serial.println("\n\nPressed OK!\n\n");
}

void button_press_cancelled()
{
	Serial.println("\n\nPressed CANCEL!\n\n");
}

void dialogbox_example()
{
	dialogbox.set_button_ok("OK", button_press_ok);
	dialogbox.set_button_cancel("CANCEL!", button_press_cancelled);
	dialogbox.show("Example Dialog Box", "Being careful not to overflow the heap and then get rubbbish on sceen - or spelling mistakes!");
}

void update_wallpaper()
{
	squixl.main_screen()->show_user_background_jpg();
}

void reconnect_wifi()
{
	// Callback used to switch DNS settings between local assigned and DCHP assigned
	WiFi.disconnect();
	delay(100);
	wifi_controller.connect();
}

void process_longitude_latitude(bool success, const String &response)
{
	try
	{
		Serial.printf("Response was: %s\n", response.c_str());

		json data = json::parse(response);

		// Serial.printf("\ncoord is array? %d, object? %d \n", data.is_array(), data.is_object());

		if (data.is_array())
		{
			bool changed = false;
			String _lon = String(data[0].value("lon", 0.0));
			String _lat = String(data[0].value("lat", 0.0));
			Serial.printf("Found coord: lon %s, lat %s\n", _lon, _lat);

			if (_lon != "0.0")
			{
				settings.config.location.lon = String(_lon);
				Serial.printf("Updated LON to %s\n", settings.config.location.lon.c_str());
				changed = true;
			}
			if (_lat != "0.0")
			{
				settings.config.location.lat = String(_lat);
				Serial.printf("Updated LAT to %s\n", settings.config.location.lat.c_str());
				changed = true;
			}

			if (changed)
			{
				squixl.current_screen()->refresh(true, true);
				audio.play_tone(1000, 10);
			}
		}
	}
	catch (json::exception &e)
	{

		Serial.println("longitude_latitude parse error:");
		Serial.println(e.what());
		Serial.printf("Response was: %s\n", response.c_str());
	}
}

void update_longitude_latitude()
{
	String url = "http://api.openweathermap.org/geo/1.0/direct?q=" + settings.config.location.city + "," + settings.config.location.state + "," + settings.config.location.country + "&limit=1&appid=" + settings.config.open_weather.api_key;
	wifi_controller.add_to_queue(url.c_str(), [](bool success, const String &response) { process_longitude_latitude(success, response); });
}

void create_ui_elements()
{
	/*
	Setup Settings Screen
	*/
	screen_settings.setup(darken565(0x5AEB, 0.5), false);

	// Settings are grouped by tabs, so we setup the tab group here with a screen size
	// and then pass it a list of strings for each group
	//
	settings_tab_group.create(0, 0, 480, 40);
	settings_tab_group.set_tabs(std::vector<std::string>{"General", "Location", "WiFi", "Snd/Hap", "Widgets", "MQTT"});
	screen_settings.set_page_tabgroup(&settings_tab_group);

	// grid layout is on a 6 column, 6 row array

	// General

	slider_backlight_timer_battery.create_on_grid(4, 1);
	slider_backlight_timer_battery.set_value_type(VALUE_TYPE::INT);
	slider_backlight_timer_battery.set_options_data(&settings.settings_backlight_timer_battery);
	settings_tab_group.add_child_ui(&slider_backlight_timer_battery, 0);

	toggle_sleep_battery.create_on_grid(2, 1, "SLEEP ON BAT");
	toggle_sleep_battery.set_toggle_text("NO", "YES");
	toggle_sleep_battery.set_options_data(&settings.setting_sleep_battery);
	settings_tab_group.add_child_ui(&toggle_sleep_battery, 0);

	slider_backlight_timer_vbus.create_on_grid(4, 1);
	slider_backlight_timer_vbus.set_value_type(VALUE_TYPE::INT);
	slider_backlight_timer_vbus.set_options_data(&settings.settings_backlight_timer_vbus);
	settings_tab_group.add_child_ui(&slider_backlight_timer_vbus, 0);

	toggle_sleep_vbus.create_on_grid(2, 1, "SLEEP ON 5V");
	toggle_sleep_vbus.set_toggle_text("NO", "YES");
	toggle_sleep_vbus.set_options_data(&settings.setting_sleep_vbus);
	settings_tab_group.add_child_ui(&toggle_sleep_vbus, 0);

	toggle_time_mode.create_on_grid(2, 1, "TIME FORMAT");
	toggle_time_mode.set_toggle_text("12H", "24H");
	toggle_time_mode.set_options_data(&settings.setting_time_24hour);
	settings_tab_group.add_child_ui(&toggle_time_mode, 0);

	toggle_date_mode.create_on_grid(2, 1, "DATE FORMAT");
	toggle_date_mode.set_toggle_text("D-M-Y", "M-D-Y");
	toggle_date_mode.set_options_data(&settings.setting_time_dateformat);
	settings_tab_group.add_child_ui(&toggle_date_mode, 0);

	toggle_wallpaper.create_on_grid(2, 1, "WALLPAPER PREF");
	toggle_wallpaper.set_toggle_text("SYS", "USER");
	toggle_wallpaper.set_options_data(&settings.setting_wallpaper);
	toggle_wallpaper.set_callback(update_wallpaper);
	settings_tab_group.add_child_ui(&toggle_wallpaper, 0);

	// Location

	// Create an Text Box the widget_ow_apikey setting
	text_loc_city.create_on_grid(6, 1, "CITY");
	text_loc_city.set_options_data(&settings.setting_loc_city);
	settings_tab_group.add_child_ui(&text_loc_city, 1);

	text_loc_state.create_on_grid(4, 1, "STATE");
	text_loc_state.set_options_data(&settings.setting_loc_state);
	settings_tab_group.add_child_ui(&text_loc_state, 1);

	text_loc_country.create_on_grid(2, 1, "COUNTRY CODE");
	text_loc_country.set_options_data(&settings.setting_loc_country);
	settings_tab_group.add_child_ui(&text_loc_country, 1);

	text_loc_lon.create_on_grid(3, 1, "LONGITUDE");
	text_loc_lon.set_options_data(&settings.setting_loc_lon);
	settings_tab_group.add_child_ui(&text_loc_lon, 1);

	text_loc_lat.create_on_grid(3, 1, "LATITUDE");
	text_loc_lat.set_options_data(&settings.setting_loc_lat);
	settings_tab_group.add_child_ui(&text_loc_lat, 1);

	button_get_lon_lat.create_on_grid(6, 1, "LOOKUP LONGITUDE & LATITUDE");
	button_get_lon_lat.set_callback(update_longitude_latitude);
	settings_tab_group.add_child_ui(&button_get_lon_lat, 1);

	slider_UTC.create_on_grid(6, 1);
	slider_UTC.set_value_type(VALUE_TYPE::INT);
	slider_UTC.set_options_data(&settings.settings_utc_offset);
	settings_tab_group.add_child_ui(&slider_UTC, 1);

	// WiFi
	toggle_OTA_updates.create_on_grid(2, 1, "ENABLE OTA");
	toggle_OTA_updates.set_toggle_text("NO", "YES");
	toggle_OTA_updates.set_options_data(&settings.setting_OTA_start);
	settings_tab_group.add_child_ui(&toggle_OTA_updates, 2);

	toggle_Notify_updates.create_on_grid(2, 1, "NOTIFY UPDATES");
	toggle_Notify_updates.set_toggle_text("NO", "YES");
	toggle_Notify_updates.set_options_data(&settings.setting_wifi_check_updates);
	settings_tab_group.add_child_ui(&toggle_Notify_updates, 2);

	toggle_verbose_wifi.create_on_grid(2, 1, "VERBOSE WIFI");
	toggle_verbose_wifi.set_toggle_text("NO", "YES");
	toggle_verbose_wifi.set_options_data(&settings.setting_wifi_extra_details);
	settings_tab_group.add_child_ui(&toggle_verbose_wifi, 2);

	text_ntpserver.create_on_grid(4, 1, "NTP SERVER");
	text_ntpserver.set_options_data(&settings.setting_ntpserver);
	settings_tab_group.add_child_ui(&text_ntpserver, 2);

	toggle_local_dns.create_on_grid(2, 1, "USE LOCAL DNS");
	toggle_local_dns.set_toggle_text("NO", "YES");
	toggle_local_dns.set_callback(reconnect_wifi);
	toggle_local_dns.set_options_data(&settings.setting_wifi_local_dns);
	settings_tab_group.add_child_ui(&toggle_local_dns, 2);

	// Sound & Haptics
	toggle_audio_ui.create_on_grid(3, 1, "UI BEEPS");
	toggle_audio_ui.set_toggle_text("NO", "YES");
	toggle_audio_ui.set_options_data(&settings.setting_audio_ui);
	settings_tab_group.add_child_ui(&toggle_audio_ui, 3);

	toggle_audio_alarm.create_on_grid(3, 1, "ALARMS");
	toggle_audio_alarm.set_toggle_text("NO", "YES");
	toggle_audio_alarm.set_options_data(&settings.setting_audio_alarm);
	settings_tab_group.add_child_ui(&toggle_audio_alarm, 3);

	slider_volume.create_on_grid(6, 1);
	slider_volume.set_value_type(VALUE_TYPE::FLOAT);
	slider_volume.set_options_data(&settings.setting_audio_volume);
	settings_tab_group.add_child_ui(&slider_volume, 3);

	toggle_haptics_enable.create_on_grid(2, 1, "HAPTICS ENABLED");
	toggle_haptics_enable.set_toggle_text("NO", "YES");
	toggle_haptics_enable.set_options_data(&settings.setting_haptics_enabled);
	settings_tab_group.add_child_ui(&toggle_haptics_enable, 3);

	// Open Weather
	// Create a Toggle from the widget_ow_enabled sewtting
	toggle_ow_enable.create_on_grid(2, 1, "OW ENABLE");
	toggle_ow_enable.set_toggle_text("NO", "YES");
	toggle_ow_enable.set_options_data(&settings.widget_ow_enabled);
	settings_tab_group.add_child_ui(&toggle_ow_enable, 4);

	// Create an Int Slider from the widget_ow_poll_interval setting
	slider_ow_refresh.create_on_grid(4, 1);
	slider_ow_refresh.set_value_type(VALUE_TYPE::INT);
	slider_ow_refresh.set_options_data(&settings.widget_ow_poll_interval);
	settings_tab_group.add_child_ui(&slider_ow_refresh, 4);

	// Create an Text Box the widget_ow_apikey setting
	text_ow_api_key.create_on_grid(6, 1, "OPEN WEATHER API KEY");
	text_ow_api_key.set_options_data(&settings.widget_ow_apikey);
	settings_tab_group.add_child_ui(&text_ow_api_key, 4);

	// RSS Feed
	// Create a Toggle from the widget_rss_enabled setting
	toggle_rss_enable.create_on_grid(2, 1, "RSS ENABLE");
	toggle_rss_enable.set_toggle_text("NO", "YES");
	toggle_rss_enable.set_options_data(&settings.widget_rss_enabled);
	settings_tab_group.add_child_ui(&toggle_rss_enable, 4);

	// Create an Int Slider from the widget_ow_poll_interval setting
	slider_rss_refresh.create_on_grid(4, 1);
	slider_rss_refresh.set_value_type(VALUE_TYPE::INT);
	slider_rss_refresh.set_options_data(&settings.widget_rss_poll_interval);
	settings_tab_group.add_child_ui(&slider_rss_refresh, 4);

	// Create an Text Box the widget_ow_apikey setting
	text_rss_feed_url.create_on_grid(6, 1, "RSS Feed URL");
	text_rss_feed_url.set_options_data(&settings.widget_rss_feed_url);
	settings_tab_group.add_child_ui(&text_rss_feed_url, 4);

	toggle_bme280_I2C_address.create_on_grid(2, 1, "BME280 I2C ADR");
	toggle_bme280_I2C_address.set_toggle_text("0x77", "0x76");
	toggle_bme280_I2C_address.set_options_data(&settings.expansion_bme_address);
	settings_tab_group.add_child_ui(&toggle_bme280_I2C_address, 4);

	// MQTT
	toggle_mqtt_enable.create_on_grid(2, 1, "MQTT ENABLED");
	toggle_mqtt_enable.set_toggle_text("NO", "YES");
	toggle_mqtt_enable.set_options_data(&settings.mqtt_enabled);
	settings_tab_group.add_child_ui(&toggle_mqtt_enable, 5);

	text_mqtt_broker_ip.create_on_grid(2, 1, "BROKER IP");
	text_mqtt_broker_ip.set_options_data(&settings.mqtt_broker_ip);
	settings_tab_group.add_child_ui(&text_mqtt_broker_ip, 5);

	text_mqtt_broker_port.create_on_grid(2, 1, "BROKER PORT");
	text_mqtt_broker_port.set_data_type(SettingsOptionBase::Type::INT);
	text_mqtt_broker_port.set_options_data(&settings.mqtt_broker_port);
	settings_tab_group.add_child_ui(&text_mqtt_broker_port, 5);

	text_mqtt_broker_username.create_on_grid(3, 1, "USERNAME");
	text_mqtt_broker_username.set_options_data(&settings.mqtt_username);
	settings_tab_group.add_child_ui(&text_mqtt_broker_username, 5);

	text_mqtt_broker_password.create_on_grid(3, 1, "PASSWORD");
	text_mqtt_broker_password.set_options_data(&settings.mqtt_password);
	settings_tab_group.add_child_ui(&text_mqtt_broker_password, 5);

	// // Screenshot stuff
	// slider_screenshot_lvl_black.create_on_grid(3, 1);
	// slider_screenshot_lvl_black.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_lvl_black.set_options_data(&settings.screenshot_black);
	// settings_tab_group.add_child_ui(&slider_screenshot_lvl_black, 5);

	// slider_screenshot_lvl_white.create_on_grid(3, 1);
	// slider_screenshot_lvl_white.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_lvl_white.set_options_data(&settings.screenshot_white);
	// settings_tab_group.add_child_ui(&slider_screenshot_lvl_white, 5);

	// slider_screenshot_lvl_gamma.create_on_grid(6, 1);
	// slider_screenshot_lvl_gamma.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_lvl_gamma.set_options_data(&settings.screenshot_gamma);
	// settings_tab_group.add_child_ui(&slider_screenshot_lvl_gamma, 5);

	// slider_screenshot_saturation.create_on_grid(3, 1);
	// slider_screenshot_saturation.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_saturation.set_options_data(&settings.screenshot_saturation);
	// settings_tab_group.add_child_ui(&slider_screenshot_saturation, 5);

	// slider_screenshot_contrast.create_on_grid(3, 1);
	// slider_screenshot_contrast.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_contrast.set_options_data(&settings.screenshot_contrast);
	// settings_tab_group.add_child_ui(&slider_screenshot_contrast, 5);

	// slider_screenshot_wb_temp.create_on_grid(3, 1);
	// slider_screenshot_wb_temp.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_wb_temp.set_options_data(&settings.screenshot_wb_temp);
	// settings_tab_group.add_child_ui(&slider_screenshot_wb_temp, 5);

	// slider_screenshot_wb_tint.create_on_grid(3, 1);
	// slider_screenshot_wb_tint.set_value_type(VALUE_TYPE::FLOAT);
	// slider_screenshot_wb_tint.set_options_data(&settings.screenshot_wb_tint);
	// settings_tab_group.add_child_ui(&slider_screenshot_wb_tint, 5);

	label_version.create(240, 460, squixl.get_version().c_str(), TFT_GREY);
	screen_settings.add_child_ui(&label_version);

	screen_settings.set_can_cycle_back_color(true);
	screen_settings.set_refresh_interval(0);

	/*
	Setup Main Screen
	*/
	screen_main.setup(TFT_BLACK, true);

	widget_battery.create(10, 0, TFT_WHITE);
	widget_battery.set_refresh_interval(5000);
	screen_main.add_child_ui(&widget_battery);

	widget_time.create(470, 10, TFT_WHITE, TEXT_ALIGN::ALIGN_RIGHT);
	widget_time.set_refresh_interval(1000);
	screen_main.add_child_ui(&widget_time);

	widget_jokes.create(10, 370, 460, 100, TFT_BLACK, 12, 0, "JOKES");
	widget_jokes.set_refresh_interval(5000);
	screen_main.add_child_ui(&widget_jokes);

	widget_rss_feeds.create(10, 260, 460, 100, TFT_BLACK, 12, 0, "RSS FEEDS");
	widget_rss_feeds.set_refresh_interval(5000);
	screen_main.add_child_ui(&widget_rss_feeds);

	widget_ow.create(245, 80, 225, 72, TFT_BLACK, 16, 0, "CURRENT WEATHER");
	widget_ow.set_refresh_interval(1000);
	screen_main.add_child_ui(&widget_ow);

	/*
	This widget will only show if a BME280 sensor is found
	*/
	widget_bme280.create(245, 160, 225, 40, TFT_BLACK, 16, 0, "BME280");
	widget_bme280.set_refresh_interval(5000); // we only want this to update every 5 seconds
	widget_bme280.set_delayed_frst_draw(2000);
	screen_main.add_child_ui(&widget_bme280);

	/*
	Setup MQTT Screen
	*/

	screen_mqtt.setup(darken565(0x5AEB, 0.5), true);
	mqtt_notifications.create(20, 20, 440, 440, "MQTT Messages", TFT_GREY);
	mqtt_notifications.set_scrollable(false, true);

	// widget_mqtt_sensors.create(10, 120, 460, 240, TFT_BLACK, 12, 0, "MQTT Messages");
	// widget_mqtt_sensors.set_refresh_interval(1000);
	screen_mqtt.add_child_ui(&mqtt_notifications);
	screen_mqtt.set_refresh_interval(0);

	screen_main.set_navigation(Directions::LEFT, &screen_mqtt, true);
	screen_main.set_navigation(Directions::DOWN, &screen_settings, true);
}

bool wifi_requirements_checked = false;
void check_wifi_requirements()
{
	wifi_requirements_checked = true;
	// This is a bit awkward - we need to see if the user has no wifi credentials,
	// or if they havn't set their country code, or f the RTC is state.
	if (!settings.has_wifi_creds() || !settings.has_country_set())
	{
		// No wifi credentials yet, so start the wifi manager
		if (!settings.has_wifi_creds())
		{
			// Don't attempt to start the webserver
			start_webserver = false;

			Serial.println("Starting WiFi AP");

			WiFi.disconnect(true);
			delay(1000);
			wifiSetup.start();
		}
		else if (!settings.has_country_set())
		{
			// wifi_controller.wifi_blocking_access = true;
			// The user has wifi credentials, but no country or UTC has been set yet.
			// Grab the location details, and then use that to get the UTC offset.
			wifi_controller.add_to_queue("https://ipapi.co/json/", [](bool success, const String &response) { squixl.get_and_update_utc_settings(success, response); });
		}
	}
}

void setup()
{
	unsigned long timer = millis();

	// Set PWM for backlight chage pump IC
	pinMode(BL_PWM, OUTPUT);
	ledcAttach(BL_PWM, 6500, LEDC_TIMER_12_BIT);
	ledcWrite(BL_PWM, 4090);

	Serial.begin(115200);
	Serial.setDebugOutput(true); // sends all log_e(), log_i() messages to USB HW CDC
	Serial.setTxTimeoutMs(0);	 // sets no timeout when trying to write to USB HW CDC

	// delay(3000);
	// squixl.log_heap("BOOT");

	if (WiFi.disconnect(true, true, 1000))
	{
		Serial.println("WIFI: Hard Disconnected at bootup");
	}

	if (!LittleFS.begin(true))
	{
		Serial.println("LittleFS failed to initialise");
		return;
	}
	else
	{
		// delay(100);
		littlefs_ready = true;
		settings.init();
		settings.load();
	}

	Wire.begin(1, 2);		 // UM square
	Wire.setBufferSize(256); // IMPORTANT: GT911 needs this
	Wire.setClock(400000);	 // Make the I2C bus fast!

	pinMode(0, INPUT_PULLUP);

	squixl.init();
	squixl.start_animation_task();

	was_asleep = squixl.was_sleeping();

	squixl.lcd.fillScreen(TFT_BLACK);
	squixl.lcd.setFont(FONT_12x16);

	squixl.mux_switch_to(MUX_STATE::MUX_I2S); // set to I2S
	audio.set_volume(settings.config.volume);

	haptics.init();

	// We only show the logo on a power cycle, not wake from sleep
	if (!was_asleep)
	{
		ioex.write(BL_EN, HIGH);
		squixl.set_backlight_level(100);
		squixl.display_logo(true);
	}

	rtc.init();
	battery.init();

	if (was_asleep)
	{
		// Wake up the peripherals because we were sleeping!
		battery.set_hibernate(false);

		// Process any POST DS callbacks onw that we have faces!
		// for (size_t i = 0; i < squixl.post_ds_callbacks.size(); i++)
		// {
		// 	if (squixl.post_ds_callbacks[i] != nullptr)
		// 	squixl.post_ds_callbacks[i]();
		// }

		int wake_reason = squixl.woke_by();

		Serial.println("Woke from sleep by " + String(wake_reason));

		if (wake_reason == 0)
		{
			// We woke from touch, so nothing really to do
		}
	}

	next_background_swap = millis();
	every_second = millis();

	wifi_controller.connect();

	// Serial.printf("\n>>> Setup done in %0.2f ms\n\n", (millis() - timer));
	check_wifi_requirements();
	// Setup delayed timer for webserver starting, to alloqw other web traffic to complete first
	delay_webserver_start = millis() + 5000;

} /* setup() */

void loop()
{
	if (wifi_controller.is_connected() && rtc.requiresNTP && millis() - ntp_time_set > 10000)
	{
		ntp_time_set = millis();
		// We have wifi credentials and country/UTC details, so set the time because it's stale.
		Serial.println("WIFI: Updating time from NTP");
		rtc.set_time_from_NTP(settings.config.location.utc_offset);
		// return;
	}

	if (squixl.switching_screens)
		return;

	// Seems we always need audio - so long as the SD card is not enabled
	if (squixl.mux_check_state(MUX_STATE::MUX_I2S))
		audio.update();

	// UI is build here, not in Setup() as setup is blocking and wont allow loop() to run until it's finished.
	// We need the logo animation and haptics/audio to be able to play which requires loop()
	if (!ui_initialised)
	{
		unsigned long timer = millis();

		ui_initialised = true;

		squixl.cache_text_sizes();

		// Func above setup() that is used to create all o ftheUI screens and controls an widgets
		create_ui_elements();

		// Continue processing startup
		if (!was_asleep)
			squixl.display_logo(false);

		if (!settings.config.first_time)
		{
			squixl.set_current_screen(&screen_main);
			screen_main.show_user_background_jpg(!was_asleep);
		}
		else
		{
			squixl.display_first_boot(true);
		}

		if (was_asleep)
		{
			squixl.set_backlight_level(0);
			ioex.write(BL_EN, HIGH);
			squixl.animate_backlight(0, 100, 500);
		}

		// Serial.printf("\n>>> UI build done in %0.2f ms\n\n", (millis() - timer));
		// squixl.log_heap("main");

		return;
	}

	if (squixl.hint_reload_wallpaper)
	{
		squixl.hint_reload_wallpaper = false; // squixl.main_screen()->show_user_background_jpg(false);
		settings.config.user_wallpaper = true;
		squixl.main_screen()->show_user_background_jpg(false);

		webserver.web_event.send("hello", "refresh", millis());
		return;
	}

	// If we have a current screen selected and it should be refreshed, refresh it!
	if (squixl.current_screen() != nullptr && squixl.current_screen()->should_refresh())
	{
		squixl.current_screen()->refresh();
	}

	// If there are any active animations running,
	// don't perocess further to allow the anims to play smoothly
	// if (animation_manager.active_animations() > 0)
	// 	return;

	// This allows desktop access to the LittleFS partition on the SQUiXL
	// littlefs_cli();

	// Touch rate is done with process_touch_full()
	// If a touch was processed, it returns true, otherwise it returns false
	if (squixl.process_touch_full())
	{
		// If 5V power had been detected, play a sound.
		if (squixl.vbus_changed())
		{
			audio.play_dock();
		}
	}

	// Process the wifi controller task queue
	// Only processes every 1 second inside it's loop
	wifi_controller.loop();

	// Process the backlight - if it gets too dark, sleepy time
	squixl.process_backlight_dimmer();

	// WiFi Setup stays running all of the time until you have configures a WiFi router to connect SQUiXL to. You can configure the credentials any time, regardless of leaving the "first time" screen.
	if (wifiSetup.running())
	{
		if (settings.config.first_time)
		{
			if (wifiSetup.wifi_ap_changed)
			{
				wifiSetup.wifi_ap_changed = false;

				if (wifiSetup.cached_message != "")
				{
					squixl.lcd.setTextColor(darken565(0x5AEB, 0.5), darken565(0x5AEB, 0.5));
					squixl.lcd.setCursor(70, 443);
					squixl.lcd.print(wifiSetup.cached_message);
				}

				squixl.lcd.setTextColor(darken565(TFT_WHITE, 0.1), darken565(0x5AEB, 0.5));

				squixl.lcd.setCursor(70, 443);
				squixl.lcd.print(wifiSetup.wifi_ap_messages);

				audio.play_tone(1000, 10);

				wifiSetup.cached_message = wifiSetup.wifi_ap_messages;
			}
		}
		else if (squixl.current_screen() == nullptr)
		{
			// We were showing the first boot screen, so no current screen is set yet.
			squixl.set_current_screen(&screen_main);
			screen_main.show_user_background_jpg(true);
		}

		if (wifiSetup.is_done())
		{
			settings.config.first_time = false;
			settings.update_wifi_credentials(wifiSetup.get_ssid(), wifiSetup.get_pass());

			// Delay required so the wifi client can land on the connected page
			delay(2000);
			wifiSetup.stop(true);
		}
	}
	else if (wifi_controller.is_connected())
	{
		if (start_webserver && millis() - delay_webserver_start > 2000)
		{
			// We want to block the web server from starting if we have starng requests in the queue that are waiting to be processed
			if (wifi_controller.items_in_queue() > 0)
			{
				delay_webserver_start = millis();
			}
			else
			{
				// ok, it's been 2 seconds since we last check and nothing is in the queue, so start the webserver.
				if (webserver.start())
				{
					start_webserver = false;
					if (settings.config.wifi_check_for_updates)
					{
						// If the user has opted in to check for firmware update notifications, kick off the check.
						// This only happens once per boot up right now.
						// TODO: Look at triggering this any time the user switches it on, if it was off?
						wifi_controller.add_to_queue("https://squixl.io/latestver", [](bool success, const String &response) { squixl.process_version(success, response); });
					}
				}
			}
		}

		// We only proess MQTT stuff if it's ben enabled by the user
		if (settings.config.mqtt.enabled)
		{
			mqtt_stuff.process_mqtt();
		}
	}

	// Finally, call non forced save on settings
	// This will only try to save every so often, and will only commit to saving if any save data has changed.
	// This is to prevent spamming the FS or causing SPI contention with the PSRAM for the frame buffer
	settings.save(false);

	// Screenie
	if (squixl.hint_take_screenshot)
	{
		squixl.take_screenshot();
	}

	screenie_tick();

} /* loop() */
