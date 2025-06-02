#include "squixl.h"
#include "ui/ui_screen.h"
#include "ui/images/logos.h"
#include "ui/icons/images/ui_icons.h"
#include "ui/controls/ui_control_textbox.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_dialogbox.h"
#include "PNGDisplay.inl"

TaskHandle_t anim_task_handler;

void SQUiXL::start_animation_task()
{
	// Start the animation task on the ESP32 using FreeRTOS.
	xTaskCreate(
		animation_task_loop,   // Task function.
		"animation_task_loop", // Name of task.
		4096,				   // Stack size.
		NULL,				   // Parameters.
		3,					   // Priority.
		&anim_task_handler	   // Task handle.
	);
}

void SQUiXL::display_logo(bool show)
{
	if (show)
	{
		if (!logo_squixl.createVirtual(280, 60, NULL, true))
		{
			Serial.println("Failed to create buffer: logo_squixl");
		}
		if (!logo_black.createVirtual(280, 60, NULL, true))
		{
			Serial.println("Failed to create buffer: logo_black");
		}
		squixl.loadPNG_into(&logo_squixl, 0, 0, squixl_logo_blue, sizeof(squixl_logo_blue));
		logo_black.fillScreen(TFT_BLACK);

		if (!by_um.createVirtual(200, 20, NULL, true))
		{
			Serial.println("Failed to create buffer: by_um");
		}
		if (!by_um_black.createVirtual(200, 20, NULL, true))
		{
			Serial.println("Failed to create buffer: by_um_black");
		}
		squixl.loadPNG_into(&by_um, 0, 0, by_um_white, sizeof(by_um_white));
		by_um_black.fillScreen(TFT_BLACK);

		for (uint8_t alpha = 0; alpha < 32; alpha++)
		{
			squixl.lcd.blendSprite(&logo_squixl, &logo_black, &logo_black, alpha);
			squixl.lcd.drawSprite(100, 200, &logo_black, 1.0, -1);
			delay(25);
		}
#ifdef AUDIO_AVAILABLE
		audio.play_wav("hello");
		audio.wait_for_finish();
#endif

		delay(500);
		for (uint8_t alpha = 0; alpha < 32; alpha++)
		{
			squixl.lcd.blendSprite(&by_um, &by_um_black, &by_um_black, alpha);
			squixl.lcd.drawSprite(140, 270, &by_um_black, 1.0, -1);
			delay(25);
		}
		haptics.play_trigger(Triggers::STARTUP);
	}
	else
	{
		by_um.fillScreen(TFT_BLACK);
		logo_squixl.fillScreen(TFT_BLACK);
		for (uint8_t alpha = 0; alpha < 32; alpha++)
		{
			squixl.lcd.blendSprite(&by_um, &by_um_black, &by_um_black, alpha);
			squixl.lcd.blendSprite(&logo_squixl, &logo_black, &logo_black, alpha);
			squixl.lcd.drawSprite(100, 200, &logo_black, 1.0, -1);
			squixl.lcd.drawSprite(140, 270, &by_um_black, 1.0, -1);
			delay(25);
		}

		logo_black.freeVirtual();
		by_um.freeVirtual();
		by_um_black.freeVirtual();
	}
}

void SQUiXL::display_first_boot(bool show)
{
	if (show)
	{
		logo_squixl.createVirtual(280, 60, NULL, true);
		squixl.loadPNG_into(&logo_squixl, 0, 0, squixl_logo_blue, sizeof(squixl_logo_blue));

		wifi_icon.createVirtual(32, 32, NULL, true);
		squixl.loadPNG_into(&wifi_icon, 0, 0, wifi_images[4], wifi_image_sizes[4]);
		squixl.loadPNG_into(&logo_squixl, 0, 0, squixl_logo_blue, sizeof(squixl_logo_blue));

		wifi_manager_content.createVirtual(480, 480, NULL, true);
		wifi_manager_content.fillScreen(darken565(0x5AEB, 0.7));

		wifi_manager_content.drawSprite(170, 20, &logo_squixl, 0.5, 0x0, DRAW_TO_RAM);

		wifi_manager_content.setFreeFont(UbuntuMono_R[2]);
		wifi_manager_content.setTextColor(darken565(TFT_WHITE, 0.1), -1);
		wifi_manager_content.setCursor(0, 100);
		wifi_manager_content.println("  Hey, you got a SQUiXL, Congratulations!\n");

		wifi_manager_content.setTextColor(darken565(TFT_WHITE, 0.3), -1);
		wifi_manager_content.println("  SQUiXL requires access to the internet,");
		wifi_manager_content.println("  so a captive portal has been launched for");
		wifi_manager_content.println("  you to setup your WiFi credentials.\n");
		wifi_manager_content.print("  Join the ");
		wifi_manager_content.setTextColor(squixl_blue, -1);
		wifi_manager_content.print("SQUiXL");
		wifi_manager_content.setTextColor(darken565(TFT_WHITE, 0.3), -1);
		wifi_manager_content.println(" WiFi AP on your phone");
		wifi_manager_content.println("  or tablet, select your WiFi SSID and");
		wifi_manager_content.println("  enter your password, then click connect.\n");

		wifi_manager_content.println("  You will only see this message once, but");
		wifi_manager_content.println("  the captive portal will remain running");
		wifi_manager_content.println("  until you setup your WiFi.\n");

		wifi_manager_content.setTextColor(TFT_WHITE, -1);
		wifi_manager_content.println("  You can skip this now by tapping the screen.\n");

		wifi_manager_content.drawSprite(20, 420, &wifi_icon, 1.0f, 0x0, DRAW_TO_RAM);

		for (uint8_t alpha = 0; alpha < 32; alpha++)
		{
			squixl.lcd.blendSprite(&wifi_manager_content, &squixl.lcd, &squixl.lcd, alpha, TFT_MAGENTA);
			delay(25);
		}

		squixl.lcd.setFreeFont(UbuntuMono_R[3]);
	}
	else
	{
		logo_squixl.freeVirtual();
		wifi_manager_content.freeVirtual();
		wifi_icon.freeVirtual();
	}
}

void SQUiXL::set_backlight_level(float pwm_level_percent)
{
	// Screen is LOW side, so 0 is full bright, and 4096 of dimmest
	uint16_t pwm_level = 4090 - (uint16_t)((pwm_level_percent / 100.0f) * 4090.0);
	ledcWrite(BL_PWM, pwm_level);
	current_backlight_pwm = pwm_level_percent;
}

void SQUiXL::animate_backlight(float from, float to, unsigned long duration)
{
	animation_manager.add_animation(new tween_animation(from, to, duration, tween_ease_t::EASE_LINEAR, [this](float val) { this->set_backlight_level(val); }));
}

void SQUiXL::process_backlight_dimmer()
{
	if (millis() - backlight_dimmer_timer > ((is_5V_detected ? settings.config.backlight_time_step_vbus : settings.config.backlight_time_step_battery) * 1000))
	{
		backlight_dimmer_timer = millis();

		// if (is_5V_detected && !settings.config.sleep_vbus)
		// {
		// 	// Never dimm the backlight or go to sleep when powered from 5V
		// 	// unless the user selected dimming on 5V in settings
		// 	return;
		// }

		if (current_backlight_pwm == 0)
		{
			if ((is_5V_detected && settings.config.sleep_vbus) || (!is_5V_detected && settings.config.sleep_battery))
			{
				go_to_sleep();
				return;
			}
		}

		animate_backlight(current_backlight_pwm, constrain(current_backlight_pwm - 33.0f, 0.0f, 100.0f), 500);
		change_cpu_frequency(false);
	}
}

uint8_t SQUiXL::cycle_next_wallpaper()
{
	settings.config.current_background++;
	if (settings.config.current_background == 6)
		settings.config.current_background = 0;

	return settings.config.current_background;
}

void SQUiXL::set_wallpaper_index(uint8_t index)
{
	settings.config.current_background = index;
}

// helper function that loads a PNG header file into a sprite
void SQUiXL::loadPNG_into(BB_SPI_LCD *sprite, int start_x, int start_y, const void *image_data, int image_data_size)
{
	int w, h, bpp;
	if (pd.getPNGInfo(&w, &h, &bpp, image_data, image_data_size))
	{
		// Serial.printf("PNG info: w %d, h %d, bpp %d\n", w, h, bpp);
		pd.loadPNG(sprite, start_x, start_y, image_data, image_data_size, 0);
		delay(25);
	}
	else
	{
		Serial.println("PNG not loaded :(");
	}
}

/// @brief Detect if 5V as gone from NO to YES. Look complicated, but we dont want to check a change in both ways, just a change from NO to YES.
/// @return if 5V was not present and now is.
bool SQUiXL::vbus_changed()
{
	bool detected = false;
	bool vbus = ioex.read(VBUS_SENSE);
	if (vbus != is_5V_detected)
	{
		if (vbus)
		{
			detected = true;
			squixl.animate_backlight(current_backlight_pwm, 100, 500);
			change_cpu_frequency(true);
		}
	}
	is_5V_detected = vbus;
	return detected;
}

// Used to get local public IP address for country/location lookup for UTC and OW
void SQUiXL::get_public_ip(bool success, const String &response)
{
	// Serial.println("Callback executed. Success: " + String(success ? "TRUE" : "FALSE") + ", Response: " + String(response));
	if (success)
	{
		std::string url = std::string("https://ipapi.co/") + response.c_str() + std::string("/json/");
		wifi_controller.add_to_queue(url, [](bool success, const String &response) { squixl.get_and_update_utc_settings(success, response); });
	}
	else
	{
		wifi_controller.wifi_blocking_access = false;
	}
}

void SQUiXL::get_and_update_utc_settings(bool success, const String &response)
{
	// Serial.println("Callback executed. Success: " + String(success ? "TRUE" : "FALSE") + ", Response: " + String(response));

	if (success && !response.isEmpty())
	{
		json data = json::parse(response);

		settings.config.city = data["city"].get<String>();
		settings.config.country = data["country_code"].get<String>();
		String utc_offset = data["utc_offset"].get<String>();

		Serial.printf("city: %s\n", settings.config.city);
		Serial.printf("country: %s\n", settings.config.country);
		Serial.printf("utc: %d\n", utc_offset);

		const char *utc_offset_data = utc_offset.c_str();

		int32_t calc_offset = ((utc_offset_data[1] - '0') * 10) + (utc_offset_data[2] - '0'); // hour
		calc_offset *= 60;
		calc_offset += ((utc_offset_data[3] - '0') * 10) + (utc_offset_data[4] - '0'); // minute
		calc_offset = (calc_offset * 60 * ((utc_offset_data[0] == '-' ? -1 : 1)));

		settings.config.utc_offset = calc_offset / 3600;

		Serial.printf("utc fixed: %d\n", settings.config.utc_offset);

		settings.save(true);

		bool obtained = false;
		uint8_t retries = 3;
		while (!obtained && retries > 0)
		{
			obtained = rtc.set_time_from_NTP(settings.config.utc_offset);
			retries--;
		}
		if (!obtained)
		{
			Serial.println("Failed to set NTP time");
		}
		else
		{
			Serial.println("Set the NTP time");
			rtc.hasTime = true;
		}
	}
	else
	{
		Serial.println("Failed to obtain UTC details");
	}

	wifi_controller.wifi_blocking_access = false;
}

bool SQUiXL::process_touch_full()
{
	if (next_touch > millis() || millis() - next_touch < touch_rate)
		return false;

	next_touch = millis();

	uint8_t move_margin_for_drag = 5;

	uint16_t pts[5][4];
	uint8_t n = xtouch.readPoints(pts);

	if (keyboard.showing)
	{
		// We don't need a fast touch rate for key pressing
		touch_rate = 100;
		if (n > 0)
		{
			backlight_dimmer_timer = millis();
			change_cpu_frequency(true);

			if (pts[0][1] < 200)
			{
				currently_selected = nullptr;
				isTouched = true;
				keyboard.show(false, nullptr);
				next_touch = millis() + 500;
				return false;
			}
			else if (!isTouched)
			{
				isTouched = true;
				startX = pts[0][0];
				startY = pts[0][1];
			}
		}
		else if (isTouched)
		{
			isTouched = false;
			keyboard.update(touch_event_t(startX, startY, TOUCH_TAP));
		}
		next_touch = millis();

		keyboard.flash_cursor();
		return true;
	}
	else if (dialogbox.is_active())
	{
		// We don't need a fast touch rate for dialogbox buttons
		touch_rate = 100;
		if (n > 0)
		{
			if (dialogbox.check_button_hit(pts[0][0], pts[0][1]))
			{
				next_touch = millis() + 500;
				audio.play_tone(500, 1);
				currently_selected = nullptr;
				isTouched = true;
				return false;
			}
		}

		next_touch = millis();
		return true;
	}
	else if (drag_lock != DRAGABLE::DRAG_BOTH)
	{
		touch_rate = 10;
	}
	else
	{
		touch_rate = 25;
	}

	if (n > 0)
	{
		if (settings.config.first_time)
		{
#ifdef AUDIO_AVAILABLE
			audio.play_tone(1000, 11);
#endif
			display_first_boot(false);
			settings.config.first_time = false;
			settings.save(false);

			return true;
		}

		backlight_dimmer_timer = millis();
		change_cpu_frequency(true);
		if (current_backlight_pwm < 100)
		{
			animate_backlight(current_backlight_pwm, 100, 500);
			current_backlight_pwm = 100;
		}

		if (!isTouched)
		{
			startX = pts[0][0];
			startY = pts[0][1];
			deltaX = 0;
			deltaY = 0;
			moved_x = pts[0][0];
			moved_y = pts[0][1];
			isTouched = true;
			prevent_long_press = false;
			touchTime = millis();

			clamp_delta_low = -20;
			clamp_delta_high = 20;

			last_touch = millis();
			drag_rate = millis();
			last_finger_move = millis();

			tab_group_index = -1;

			drag_lock = DRAGABLE::DRAG_BOTH;

			if (current_screen() != nullptr)
			{
				tab_group_index = current_screen()->get_tab_group_index();
				if (tab_group_index > -1)
				{
					if (current_screen()->ui_tab_group->process_touch(touch_event_t(pts[0][0], pts[0][1], TOUCH_TAP)))
					{
						isTouched = false;
						next_touch = millis() + 250;
						currently_selected = nullptr;
						// Serial.printf("BLOCKED!!!! @ millis() %u -> next %u\n", millis(), next_touch);
						return false;
					}
					else
					{
						currently_selected = current_screen()->ui_tab_group->find_touched_element(pts[0][0], pts[0][1], tab_group_index);
					}
				}
				else
				{
					currently_selected = current_screen()->find_touched_element(pts[0][0], pts[0][1], -1);
				}
			}

			if (current_screen() == nullptr || currently_selected == nullptr)
			{
				Serial.println("current_screen() == nullptr || currently_selected == nullptr");
				isTouched = false;
				return true;
			}
		}
		else if (isTouched)
		{
			if (last_was_long)
				return true;

			deltaX = int16_t(pts[0][0]) - int16_t(startX);
			deltaY = int16_t(pts[0][1]) - int16_t(startY);

			int16_t moved_much_x = pts[0][0] - moved_x;
			int16_t moved_much_y = pts[0][1] - moved_y;

			moved_x = pts[0][0];
			moved_y = pts[0][1];

			// Serial.printf("updated touch: %d,%d - delta: %d,%d\n", pts[0][0], pts[0][1], deltaX, deltaY);

			last_touch = millis();

			if (abs(deltaX) > move_margin_for_drag || abs(deltaY) > move_margin_for_drag)
			{
				// the user is dragging their finger - if the currently selected item is not the screen, we need to see if it's draggable
				if (currently_selected != nullptr && currently_selected != current_screen())
				{
					// the element is not draggable, so fallback to the screen so the user can grab the screen
					if (currently_selected->is_dragable() == DRAGABLE::DRAG_NONE)
					{
						currently_selected = current_screen();
						// Serial.println("switched element to screen because the user is dragging.");
					}
					else
					{
						// Ok, the item is draggable, but is it draggable in the direction the user is dragging?
						// The user is dragging horizontally
						if (abs(deltaX) > abs(deltaY) && currently_selected->is_dragable() != DRAG_HORIZONTAL)
						{
							currently_selected = current_screen();
							// Serial.println("element can't be dragged horizontally, so switcing to screen");
						}
						else if (abs(deltaX) < abs(deltaY) && currently_selected->is_dragable() != DRAG_VERTICAL)
						{
							currently_selected = current_screen();
							// Serial.println("element can't be dragged vertically, so switcing to screen");
						}
					}
				}
			}

			if (currently_selected == current_screen() && (abs(deltaX) > move_margin_for_drag || abs(deltaY) > move_margin_for_drag))
			{
				if (drag_lock == DRAGABLE::DRAG_BOTH)
				{
					drag_lock = (abs(deltaX) > abs(deltaY)) ? DRAGABLE::DRAG_HORIZONTAL : DRAGABLE::DRAG_VERTICAL;
					current_screen()->adjust_navigation_range(drag_lock, &clamp_delta_low, &clamp_delta_high);
					// Serial.printf("Set drag constraints to %d <-> %d\n", clamp_delta_low, clamp_delta_high);
				}

				// Calculate the range of movement based on the locked axis and if the screen has linked neighbours
				if (drag_lock == DRAGABLE::DRAG_HORIZONTAL)
				{
					deltaX = constrain(deltaX, clamp_delta_low, clamp_delta_high);
					moved_much_x = constrain(moved_much_x, clamp_delta_low, clamp_delta_high);
					currently_selected->process_touch(touch_event_t(deltaX, 0, SCREEN_DRAG_H, moved_much_x, 0));
				}
				else
				{
					deltaY = constrain(deltaY, clamp_delta_low, clamp_delta_high);
					moved_much_y = constrain(moved_much_y, clamp_delta_low, clamp_delta_high);
					currently_selected->process_touch(touch_event_t(0, deltaY, SCREEN_DRAG_V, 0, moved_much_y));
				}
				prevent_long_press = true;
				// Serial.printf("touch: dragging @ %u\n", millis());
				return true;
			}
			else if (currently_selected != nullptr && currently_selected->is_dragable() != DRAGABLE::DRAG_NONE && (abs(deltaX) > move_margin_for_drag || abs(deltaY) > move_margin_for_drag))
			{
				currently_selected->process_touch(touch_event_t(moved_x, moved_y, TOUCH_DRAG));
				prevent_long_press = true;
			}
			else if (!prevent_long_press && last_touch - touchTime > 600)
			{
				// might be a long click?
				if (currently_selected != nullptr)
				{
					if (currently_selected->process_touch(touch_event_t(moved_x, moved_y, TOUCH_LONG)))
					{
						last_was_click = false;
						last_was_long = true;
					}
				}
			}
		}
	}
	else if (isTouched)
	{
		isTouched = false;

		if (last_was_long)
		{
			last_was_long = false;
			return true;
		}

		last_touch = millis();

		deltaX = int16_t(moved_x) - int16_t(startX);
		deltaY = int16_t(moved_y) - int16_t(startY);

		uint16_t deltaX_abs = abs(deltaX);
		uint16_t deltaY_abs = abs(deltaY);

		if (currently_selected != nullptr)
		{

			// If the delta from first touch to current is small enough to suggest a click, process a click or long click
			if ((abs(deltaX) < 5 && abs(deltaY) < 5))
			{
				dbl_touch[0] = dbl_touch[1];
				dbl_touch[1] = millis();

				bool double_click = (dbl_touch[1] - dbl_touch[0] < 200);
				if (double_click)
				{
					// block 2 dbl clicks in a row from 3 clicks
					dbl_touch[1] = 0;

					last_was_click = false;

					if (currently_selected->process_touch(touch_event_t(moved_x, moved_y, TOUCH_DOUBLE)))
					{
						return true;
						;
					}
				}
				else
				{
					last_was_click = true;
				}
			}
			// If the current screen is blocked from dragging, if the distance from first touch to last is enough to suggest a swipe, pass a swipe to the current ui element
			// else if (currently_selected == current_screen() && (deltaX_abs > 25 || deltaY_abs > 25))
			// {
			// 	// Calculate swipe dir to pass on
			// 	int8_t dir = -1;
			// 	if (deltaY_abs > deltaX_abs)
			// 		dir = (deltaY < 0) ? (int)TouchEventType::TOUCH_SWIPE_UP : (int)TouchEventType::TOUCH_SWIPE_DOWN;
			// 	else
			// 		dir = (deltaX > 0) ? (int)TouchEventType::TOUCH_SWIPE_RIGHT : (int)TouchEventType::TOUCH_SWIPE_LEFT;

			// 	if (currently_selected->process_touch(touch_event_t(moved_x, moved_y, (TouchEventType)dir, deltaX, deltaY)))
			// 	{
			// 		return true;
			// 	}
			// 	else if (currently_selected->ui_parent != nullptr)
			// 	{
			// 		if (currently_selected->ui_parent->process_touch(touch_event_t(moved_x, moved_y, (TouchEventType)dir, deltaX, deltaY)))
			// 		{
			// 			return true;
			// 		}
			// 	}
			// }
		}
	}

	// If there was a pervious click, and the time past has been longer than what a double click would trigger, process the original single click
	if (!isTouched && drag_lock != DRAGABLE::DRAG_BOTH && currently_selected != nullptr)
	{
		// Serial.printf("touch: let go of drag @ %u\n", millis());
		currently_selected->process_touch(touch_event_t(moved_x, moved_y, TOUCH_UNKNOWN));
		currently_selected = nullptr;
		drag_lock = DRAGABLE::DRAG_BOTH;
		clamp_delta_low = -20;
		clamp_delta_high = 20;
	}
	if (last_was_click && millis() - last_touch > 150)
	{
		last_was_click = false;
		if (currently_selected != nullptr)
		{
			if (currently_selected->process_touch(touch_event_t(moved_x, moved_y, TOUCH_TAP)))
			{
				current_screen()->refresh(true);
			}
		}
	}

	return true;
}

void SQUiXL::set_current_screen(ui_screen *screen)
{
	if (_current_screen != nullptr)
		_current_screen->clear_buffers();

	_current_screen = screen;
	_current_screen->create_buffers();

	if (_main_screen == nullptr)
	{
		// first screen being added, so make it the main screen
		_main_screen = screen;

		Serial.printf("Set main screen - is same as current? %d\n", (_main_screen == _current_screen));
	}

	// log_heap("Screen Switch");
}

ui_screen *SQUiXL::current_screen()
{
	return _current_screen;
}

ui_screen *SQUiXL::main_screen()
{
	return _main_screen;
}

void SQUiXL::toggle_settings()
{
	showing_settings = !showing_settings;
}

void SQUiXL::change_cpu_frequency(bool increase)
{
	// Go full speed on the mega hertz
	if (increase)
		current_cpu_frequency = CPU_FREQ_MAX;

	// Wifi will crash if the CPU speed is less than 80
	else if (current_cpu_frequency > CPU_FREQ_LOW_WIFI)
		current_cpu_frequency = CPU_FREQ_LOW_WIFI;

	if (getCpuFrequencyMhz() != current_cpu_frequency)
		setCpuFrequencyMhz(current_cpu_frequency);
}

bool SQUiXL::was_sleeping() { return (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1); }

int SQUiXL::woke_by()
{
	return 0;
}

void SQUiXL::go_to_sleep()
{
	// if the web server is running, we don't want to go to sleep...
	if (webserver.is_running())
	{
		webserver.stop(false);
		Serial.println("Stopping the Web server because I am going to sleep");
		return;
	}

	if (wifi_controller.is_connected())
		wifi_controller.disconnect(true);

	if (wifi_controller.is_busy())
	{
		Serial.println("Cant sleep yet, wifi is busy!");
		return;
	}

	// Diable the backlight to save battery
	set_backlight_level(0);
	ioex.write(BL_EN, LOW);

	// Disable haptics to save battery
	ioex.write(HAPTICS_EN, LOW);

	// Turn off the IO MUX to save battery (Active LOW)
	ioex.write(MUX_EN, HIGH);

	for (size_t i = 0; i < pre_ds_callbacks.size(); i++)
	{
		if (pre_ds_callbacks[i] != nullptr)
			pre_ds_callbacks[i]();
	}

	// Dont call this if the task was not created!
	wifi_controller.kill_controller_task();

	vTaskDelete(anim_task_handler);

	battery.set_hibernate(true);

	settings.save(true);
	delay(200);

	LittleFS.end();

	esp_sleep_enable_ext1_wakeup(WAKE_REASON_TOUCH, ESP_EXT1_WAKEUP_ANY_LOW);
	esp_deep_sleep_start();
}

void SQUiXL::process_version(bool success, const String &response)
{
	try
	{
		json data = json::parse(response);

		uint16_t latest_version = data["latest_version"];

		Serial.printf("\n ***Latest Version: %d, Build Version: %d, Should notify? %s\n\n", latest_version, version_build, String(latest_version > version_build ? "YES!" : "no"));
		version_latest = latest_version;
	}
	catch (json::exception &e)
	{
		Serial.println("Version Check parse error:");
		Serial.println(e.what());
	}
}

void SQUiXL::take_screenshot()
{
	audio.play_dock();
	hint_take_screenshot = save_png(&lcd);
	if (webserver.is_running())
		webserver.web_event.send("hello", "refresh", millis());
}

void SQUiXL::split_text_into_lines(const String &text, int max_chars_per_line, std::vector<String> &lines)
{
	String currentLine = "";
	int pos = 0;

	while (pos < text.length())
	{
		// Find the next space starting from pos.
		int spaceIndex = text.indexOf(' ', pos);
		String word;

		// If no more spaces, grab the rest of the string.
		if (spaceIndex == -1)
		{
			word = text.substring(pos);
			pos = text.length();
		}
		else
		{
			word = text.substring(pos, spaceIndex);
			pos = spaceIndex + 1; // Move past the space.
		}

		// If currentLine is empty, start it with the word.
		if (currentLine.length() == 0)
		{
			currentLine = word;
		}
		// Otherwise, check if adding the next word (with a space) would exceed the limit.
		else if (currentLine.length() + 1 + word.length() <= max_chars_per_line)
		{
			currentLine += " " + word;
		}
		// If it would exceed the limit, push the current line and start a new one.
		else
		{
			currentLine.replace("\n", " ");
			currentLine.replace("\r", " ");
			lines.push_back(currentLine);
			currentLine = word;
		}
	}

	// Add the last line if not empty.
	if (currentLine.length() > 0)
	{
		currentLine.replace("\n", " ");
		currentLine.replace("\r", " ");
		lines.push_back(currentLine);
	}
}

SQUiXL squixl;