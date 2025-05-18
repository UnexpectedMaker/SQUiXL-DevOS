#include "squixl.h"

#include "ui/icons/images/ui_icons.h"
#include "ui/ui_screen.h"

#include "ui/widgets/widget_openweather.h"
#include "ui/widgets/widget_jokes.h"
#include "ui/widgets/widget_time.h"
#include "ui/widgets/widget_mqtt_sensors.h"
#include "ui/widgets/widget_battery.h"
#include "ui/widgets/widget_wifimanager.h"

#include "ui/controls/ui_control_button.h"
#include "ui/controls/ui_control_toggle.h"
#include "ui/controls/ui_control_slider.h"
#include "ui/controls/ui_control_textbox.h"

#include "ui/controls/ui_control_tabgroup.h"

#include "mqtt/mqtt.h"
#include "utils/littlefs_cli.h"

unsigned long next_background_swap = 0;
unsigned long every_second = 0;

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
// Time
ui_control_toggle toggle_time_mode;
ui_control_toggle toggle_date_mode;
ui_control_slider slider_UTC;
// WiFi
ui_control_toggle toggle_OTA_updates;
ui_control_toggle toggle_Notify_updates;

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
ui_control_textbox text_ow_country;
ui_control_textbox text_ow_city;
// Screenshot stuff
ui_control_toggle toggle_screenshot_enable;
ui_control_slider slider_screenshot_wb_temp;
ui_control_slider slider_screenshot_wb_tint;
ui_control_slider slider_screenshot_lvl_black;
ui_control_slider slider_screenshot_lvl_white;
ui_control_slider slider_screenshot_lvl_gamma;
ui_control_slider slider_screenshot_saturation;
ui_control_slider slider_screenshot_contrast;

void create_ui_elements()
{

	// screen_wifi_manager.setup(0x5AEB, false);
	// widget_wifimanager.create();
	// // widget_wifimanager.set_back_screen(&screen_main);
	// screen_wifi_manager.add_child_ui(&widget_wifimanager);
	// wifiSetup.set_screen(&screen_wifi_manager);

	/*
	Setup Settings Screen
	*/
	screen_settings.setup(0x5AEB, false);

	// Settings are grouped by tabs, so we setup the tab group here with a screen size
	// and then pass it a list of strings for each group
	//
	settings_tab_group.create(0, 0, 480, 40);
	settings_tab_group.set_tabs(std::vector<std::string>{"General", "WiFi", "Sound", "Haptics", "Weather", "Screenie"});
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

	toggle_time_mode.create_on_grid(3, 1, "TIME FORMAT");
	toggle_time_mode.set_toggle_text("12H", "24H");
	toggle_time_mode.set_options_data(&settings.setting_time_24hour);
	settings_tab_group.add_child_ui(&toggle_time_mode, 0);

	toggle_date_mode.create_on_grid(3, 1, "DATE FORMAT");
	toggle_date_mode.set_toggle_text("D-M-Y", "M-D-Y");
	toggle_date_mode.set_options_data(&settings.setting_time_dateformat);
	settings_tab_group.add_child_ui(&toggle_date_mode, 0);

	slider_UTC.create_on_grid(6, 1);
	slider_UTC.set_value_type(VALUE_TYPE::INT);
	slider_UTC.set_options_data(&settings.settings_utc_offset);
	settings_tab_group.add_child_ui(&slider_UTC, 0);

	// WiFi
	toggle_OTA_updates.create_on_grid(3, 1, "ENABLE OTA");
	toggle_OTA_updates.set_toggle_text("NO", "YES");
	toggle_OTA_updates.set_options_data(&settings.setting_OTA_start);
	settings_tab_group.add_child_ui(&toggle_OTA_updates, 1);

	toggle_Notify_updates.create_on_grid(3, 1, "NOTIFY UPDATES");
	toggle_Notify_updates.set_toggle_text("NO", "YES");
	toggle_Notify_updates.set_options_data(&settings.setting_wifi_check_updates);
	settings_tab_group.add_child_ui(&toggle_Notify_updates, 1);

	// Sound
	toggle_audio_ui.create_on_grid(3, 1, "UI BEEPS");
	toggle_audio_ui.set_toggle_text("NO", "YES");
	toggle_audio_ui.set_options_data(&settings.setting_audio_ui);
	settings_tab_group.add_child_ui(&toggle_audio_ui, 2);

	toggle_audio_alarm.create_on_grid(3, 1, "ALARMS");
	toggle_audio_alarm.set_toggle_text("NO", "YES");
	toggle_audio_alarm.set_options_data(&settings.setting_audio_alarm);
	settings_tab_group.add_child_ui(&toggle_audio_alarm, 2);

	slider_volume.create_on_grid(6, 1);
	slider_volume.set_value_type(VALUE_TYPE::FLOAT);
	slider_volume.set_options_data(&settings.setting_audio_volume);
	settings_tab_group.add_child_ui(&slider_volume, 2);

	// Haptics

	toggle_haptics_enable.create_on_grid(2, 1, "ENABLED");
	toggle_haptics_enable.set_toggle_text("NO", "YES");
	toggle_haptics_enable.set_options_data(&settings.setting_haptics_enabled);
	settings_tab_group.add_child_ui(&toggle_haptics_enable, 3);

	// Open Weather
	// Create a Toggle frmo the widget_ow_enabled sewtting
	toggle_ow_enable.create_on_grid(2, 1, "ENABLE");
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

	// Create an Text Box the widget_ow_apikey setting
	text_ow_country.create_on_grid(2, 1, "COUNTRY CODE");
	text_ow_country.set_options_data(&settings.setting_country);
	settings_tab_group.add_child_ui(&text_ow_country, 4);

	// Create an Text Box the widget_ow_apikey setting
	text_ow_city.create_on_grid(4, 1, "CITY");
	text_ow_city.set_options_data(&settings.setting_city);
	settings_tab_group.add_child_ui(&text_ow_city, 4);

	// Screenshot stuff
	slider_screenshot_lvl_black.create_on_grid(3, 1);
	slider_screenshot_lvl_black.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_lvl_black.set_options_data(&settings.screenshot_black);
	settings_tab_group.add_child_ui(&slider_screenshot_lvl_black, 5);

	slider_screenshot_lvl_white.create_on_grid(3, 1);
	slider_screenshot_lvl_white.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_lvl_white.set_options_data(&settings.screenshot_white);
	settings_tab_group.add_child_ui(&slider_screenshot_lvl_white, 5);

	slider_screenshot_lvl_gamma.create_on_grid(6, 1);
	slider_screenshot_lvl_gamma.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_lvl_gamma.set_options_data(&settings.screenshot_gamma);
	settings_tab_group.add_child_ui(&slider_screenshot_lvl_gamma, 5);

	slider_screenshot_saturation.create_on_grid(3, 1);
	slider_screenshot_saturation.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_saturation.set_options_data(&settings.screenshot_saturation);
	settings_tab_group.add_child_ui(&slider_screenshot_saturation, 5);

	slider_screenshot_contrast.create_on_grid(3, 1);
	slider_screenshot_contrast.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_contrast.set_options_data(&settings.screenshot_contrast);
	settings_tab_group.add_child_ui(&slider_screenshot_contrast, 5);

	slider_screenshot_wb_temp.create_on_grid(3, 1);
	slider_screenshot_wb_temp.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_wb_temp.set_options_data(&settings.screenshot_wb_temp);
	settings_tab_group.add_child_ui(&slider_screenshot_wb_temp, 5);

	slider_screenshot_wb_tint.create_on_grid(3, 1);
	slider_screenshot_wb_tint.set_value_type(VALUE_TYPE::FLOAT);
	slider_screenshot_wb_tint.set_options_data(&settings.screenshot_wb_tint);
	settings_tab_group.add_child_ui(&slider_screenshot_wb_tint, 5);

	screen_settings.set_can_cycle_back_color(true);
	screen_settings.set_refresh_interval(0);

	/*
	Setup Main Screen
	*/
	screen_main.setup(TFT_BLACK, true);

	widget_battery.create(10, 0, TFT_WHITE);
	widget_battery.set_refresh_interval(1000);
	screen_main.add_child_ui(&widget_battery);

	widget_time.create(470, 10, TFT_WHITE, TEXT_ALIGN::ALIGN_RIGHT);
	widget_time.set_refresh_interval(1000);
	screen_main.add_child_ui(&widget_time);

	widget_jokes.create(10, 370, 460, 100, TFT_BLACK, 12, 0, "JOKES");
	widget_jokes.set_refresh_interval(5000);
	screen_main.add_child_ui(&widget_jokes);

	widget_ow.create(245, 80, 225, 72, TFT_BLACK, 16, 0, "CURRENT WEATHER");
	widget_ow.set_title_alignment(TEXT_ALIGN::ALIGN_LEFT);
	widget_ow.set_refresh_interval(1000);
	screen_main.add_child_ui(&widget_ow);

	/*
	Setup MQTT Screen
	*/

	screen_mqtt.setup(TFT_BLUE, true);
	widget_mqtt_sensors.create(10, 120, 460, 240, TFT_BLACK, 12, 0, "MQTT Sensors");
	widget_mqtt_sensors.set_refresh_interval(1000);
	screen_mqtt.add_child_ui(&widget_mqtt_sensors);

	screen_main.set_navigation(Directions::LEFT, &screen_mqtt, true);
	screen_main.set_navigation(Directions::DOWN, &screen_settings, true);
}

void setup()
{
	unsigned long timer = millis();

	// Set PWM for backlight chage pump IC
	pinMode(BL_PWM, OUTPUT);
	ledcAttach(BL_PWM, 6000, LEDC_TIMER_12_BIT);

	Serial.begin(115200);
	Serial.setDebugOutput(true); // sends all log_e(), log_i() messages to USB HW CDC
	Serial.setTxTimeoutMs(0);	 // sets no timeout when trying to write to USB HW CDC

	// delay(3000);
	// squixl.log_heap("BOOT");

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

	// This is a bit awkward - we need to see if the user has no wifi credentials,
	// or if they havn't set their country code, or f teh RTC is state.
	if (rtc.requiresNTP || !settings.has_wifi_creds() || !settings.has_country_set())
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
			// The user has wifi credentials, but no country or UTC has been set yet.
			// Grab our public IP address, and then get out UTC offset and country and suburb.
			wifi_controller.add_to_queue("http://api.ipify.org", [](bool success, const String &response) { squixl.get_public_ip(success, response); });
		}
		else if (rtc.requiresNTP)
		{
			// We have wifi credentials and country/UTC details, so set the time because it's stale.
			rtc.set_time_from_NTP(settings.config.utc_offset);
		}
	}
	else
	{
		if (settings.config.wifi_check_for_updates)
		{
			// If the user has opted in to check for firmware update notifications, kick off the check.
			// This only happens once per boot up right now.
			// TODO: Look at triggering this any time the user switches it on, if it was off?
			wifi_controller.add_to_queue("https://squixl.io/latestver", [](bool success, const String &response) { squixl.process_version(success, response); });
		}
	}

	Serial.printf("\n>>> Setup done in %0.2f ms\n\n", (millis() - timer));

	// Setup delayed timer for webserver starting, to alloqw other web traffic to complete first
	delay_webserver_start = millis();

} /* setup() */

void loop()
{
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
			screen_main.show_random_background(!was_asleep);
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

		Serial.printf("\n>>> UI build done in %0.2f ms\n\n", (millis() - timer));
		// squixl.log_heap("main");

		return;
	}

	// Screenie
	if (squixl.hint_take_screenshot)
	{
		squixl.take_screenshot();
	}

	// If we have a current screen selected and it should be refreshed, refresh it!
	if (squixl.current_screen() != nullptr && squixl.current_screen()->should_refresh())
	{
		squixl.current_screen()->refresh();
	}

	// If there are any active animations running,
	// don't perocess further to allow the anims to play smoothly
	if (animation_manager.active_animations() > 0)
		return;

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

	// Process the backlight - if it gets too dark, sleepy time
	squixl.process_backlight_dimmer();

	// Process the wifi controller task queue
	// Only processes every 1 second inside it's loop
	wifi_controller.loop();

	// This is used if you sattempt to setup your wifi credentials from the first boot screen or if you setup credentials while you are doing other things in SQUiXL.
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

				// squixl.lcd.fillRect(80, 390, 400, 80, darken565(0x5AEB, 0.5));
				squixl.lcd.setCursor(70, 443);
				squixl.lcd.print(wifiSetup.wifi_ap_messages);

				audio.play_tone(1000, 10);

				wifiSetup.cached_message = wifiSetup.wifi_ap_messages;
			}
		}
		else if (squixl.current_screen() == nullptr)
		{
			// We were showing teh first boot screen, so no current screen is set yet.
			squixl.set_current_screen(&screen_main);
			screen_main.show_random_background(true);
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
		if (start_webserver && millis() - delay_webserver_start > 5000)
		{
			start_webserver = false;
			webserver.start();
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

} /* loop() */
