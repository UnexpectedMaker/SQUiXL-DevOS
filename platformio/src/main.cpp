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
ui_control_toggle toggle_time_mode;
ui_control_toggle toggle_date_mode;
ui_control_toggle toggle_OTA_updates;

// Open Weather
ui_control_toggle toggle_ow_enable;
ui_control_slider slider_ow_refresh;
ui_control_textbox text_ow_api_key;

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

		// screen_wifi_manager.setup(0x5AEB, false);
		// widget_wifimanager.create();
		// screen_wifi_manager.add_child_ui(&widget_wifimanager);
		// wifiSetup.set_screen(&screen_wifi_manager);

		screen_settings.setup(0x5AEB, false);

		// // Settings screen
		// button_hello.create(20, 20, 120, 40, "Button");
		// screen_settings.add_child_ui(&button_hello);

		// ow_group.set_grid_padding(5);
		// ow_group.create_on_grid(0, 0, 3, 2, "OPEN WEATHER");
		// ow_group.set_touchable(false);
		// screen_settings.add_child_ui(&ow_group);

		toggle_OTA_updates.create_on_grid(0, 0, 1, 1, "OTA UPDATES");
		toggle_OTA_updates.set_toggle_text("NO", "YES");
		toggle_OTA_updates.set_options_data(&settings.setting_OTA_start);
		screen_settings.add_child_ui(&toggle_OTA_updates);

		// toggle_time_mode.create(160, 10, 140, 60, "TIME FORMAT");
		toggle_time_mode.create_on_grid(1, 0, 1, 1, "TIME FORMAT");
		toggle_time_mode.set_toggle_text("12H", "24H");
		toggle_time_mode.set_options_data(&settings.setting_time_24hour);
		screen_settings.add_child_ui(&toggle_time_mode);

		// toggle_date_mode.create(320, 10, 140, 60, "DATE FORMAT");
		toggle_date_mode.create_on_grid(2, 0, 1, 1, "DATE FORMAT");
		toggle_date_mode.set_toggle_text("D-M-Y", "M-D-Y");
		toggle_date_mode.set_options_data(&settings.setting_time_dateformat);
		screen_settings.add_child_ui(&toggle_date_mode);

		// toggle_ow_enable.create(20, 90, 120, 60, "OW ENABLE");
		toggle_ow_enable.create_on_grid(0, 1, 1, 1, "OW ENABLE");
		toggle_ow_enable.set_toggle_text("NO", "YES");
		toggle_ow_enable.set_options_data(&settings.widget_ow_enabled);
		screen_settings.add_child_ui(&toggle_ow_enable);

		// Create an Int Slider from the widget_ow_poll_interval setting
		slider_ow_refresh.create_on_grid(1, 1, 2, 1);
		slider_ow_refresh.set_value_type(VALUE_TYPE::INT);
		slider_ow_refresh.set_options_data(&settings.widget_ow_poll_interval);
		screen_settings.add_child_ui(&slider_ow_refresh);

		// Create an Text Box the widget_ow_apikey setting
		text_ow_api_key.create_on_grid(0, 2, 3, 1, "OW API KEY");
		text_ow_api_key.set_options_data(&settings.widget_ow_apikey);
		screen_settings.add_child_ui(&text_ow_api_key);

		// example_slider2.create(20, 160, 440, 60);
		// example_slider2.set_min_max(50, 250, 5);
		// // example_slider2.set_suffix("%");
		// example_slider2.set_value_type(VALUE_TYPE::INT);
		// example_slider2.set_prefix("$");
		// screen_settings.add_child_ui(&example_slider2);

		screen_settings.set_can_cycle_back_color(true);
		// screen_settings.refresh(true);

		// main screen

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

		// MQTT Screen

		screen_mqtt.setup(TFT_BLUE, true);
		widget_mqtt_sensors.create(10, 120, 460, 240, TFT_BLACK, 12, 0, "MQTT Sensors");
		widget_mqtt_sensors.set_refresh_interval(1000);
		screen_mqtt.add_child_ui(&widget_mqtt_sensors);
		// screen_mqtt.refresh(true);

		screen_main.set_navigation(Directions::LEFT, &screen_mqtt, true);
		screen_main.set_navigation(Directions::DOWN, &screen_settings, true);

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
		squixl.log_heap("main");

		return;
	}

	if (squixl.current_screen() != nullptr && squixl.current_screen()->should_refresh())
	{
		squixl.current_screen()->refresh();
	}

	if (animation_manager.active_animations() > 0)
		return;

	// This allows desktop access to the LittleFS partition on the SQUiXL
	// littlefs_cli();

	// touch rate is done with process_touch - if it was processed, it returns true,
	// otherwise it returns false
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
