#include "ui/widgets/widget_battery.h"
#include "ui/icons/images/ui_icons.h"
#include "web/wifi_controller.h"
#include "web/wifi_setup.h"

#include "ui/ui_screen.h"

void widgetBattery::create(int16_t x, int16_t y, uint16_t color)
{
	_x = x;
	_y = y;
	_c = color;
	_w = 280;
	_h = 64;
}

void widgetBattery::load_icons()
{
	// Create the required sprites
	_sprite_content.createVirtual(_w, _h, NULL, true);

	// Load icons
	for (int i = 0; i < battery_images_count; i++)
	{
		battery_icons[i].createVirtual(32, 32, NULL, true);
		squixl.loadPNG_into(&battery_icons[i], 0, 0, battery_images[i], battery_image_sizes[i]);
	}

	for (int i = 0; i < wifi_images_count; i++)
	{
		wifi_icons[i].createVirtual(32, 32, NULL, true);
		squixl.loadPNG_into(&wifi_icons[i], 0, 0, wifi_images[i], wifi_image_sizes[i]);
	}

	Serial.println("Loaded battery icons");
}

void widgetBattery::capture_clean_sprite()
{
	// squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
	is_dirty_hard = false;
}

bool widgetBattery::redraw(uint8_t fade_amount, int8_t tab_group)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (!icons_loaded)
	{
		icons_loaded = true;
		load_icons();

		next_update = 0;
		is_busy = false;
		return false;
	}

	// if (is_dirty_hard)
	// {
	// 	_sprite_content.fillScreen(TFT_MAGENTA);
	// 	// _sprite_clean.fillScreen(TFT_MAGENTA);

	// 	delay(10);
	// 	is_dirty_hard = false;
	// }

	_sprite_content.fillScreen(TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(TFT_WHITE, -1);

	_sprite_content.setCursor(40, (settings.config.show_extra_wifi_details ? 15 : 20));

	if (wifi_controller.is_connected())
	{
		_sprite_content.drawSprite(0, 0, &wifi_icons[4], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.print(WiFi.localIP());
		if (settings.config.show_extra_wifi_details)
		{
			_sprite_content.print(" (");
			_sprite_content.print(WiFi.getHostname());
			_sprite_content.print(")");
			if (!squixl.update_available())
			{
				_sprite_content.setCursor(40, (settings.config.show_extra_wifi_details ? 29 : 34));
				_sprite_content.print(WiFi.SSID());
				_sprite_content.print(" CH ");
				_sprite_content.print(WiFi.channel());
				_sprite_content.print(" RSSI ");
				_sprite_content.print(WiFi.RSSI());
			}
		}
		if (squixl.update_available())
		{
			_sprite_content.setCursor(40, (settings.config.show_extra_wifi_details ? 29 : 34));
			_sprite_content.setTextColor(TFT_GREEN, -1);
			_sprite_content.print("NEW FW @ https://squixl.io/up/");
			_sprite_content.setTextColor(TFT_WHITE, -1);
		}
	}
	else if (settings.has_wifi_creds())
	{
		_sprite_content.drawSprite(0, 0, &wifi_icons[0], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.print("Disconnected");
	}

	else
	{
		_sprite_content.drawSprite(0, 0, &wifi_icons[0], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.print("WiFi not configured");
	}

	_sprite_content.setCursor(40, 52);
	if (squixl.vbus_present())
	{
		// _sprite_content.drawSprite(0, 0, &wifi_icons[wifi_controller.is_connected() ? 4 : 0], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.drawSprite(0, 32, &battery_icons[0], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.print((int)battery.get_percent());
		_sprite_content.print("% ");
	}
	else
	{
		float vbat = battery.get_voltage();
		float vbat_mapped = mapFloat(max(vbat, 3.0f), 2.9f, 4.0f, 1.0f, 4.0f);
		uint8_t bat_icon_index = (int)vbat_mapped;

		_sprite_content.drawSprite(0, 32, &battery_icons[bat_icon_index], 1.0f, 0x0, DRAW_TO_RAM);

		_sprite_content.printf("%0.1f", vbat);
		_sprite_content.print("V ");
		_sprite_content.print((int)battery.get_percent());
		_sprite_content.print("% ");
		_sprite_content.print(getCpuFrequencyMhz());
		_sprite_content.print("MHz");
	}

	// if (fade_amount < 32)
	// {
	// 	squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
	// 	ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	// }
	// else
	// {
	// squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, 32);
	ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
	next_refresh = millis();
	// }

	is_dirty = false;
	is_busy = false;
	next_refresh = millis();

	return false;
}

bool widgetBattery::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();

				// Show/Hide SSID (and beep)
				settings.config.show_extra_wifi_details = !settings.config.show_extra_wifi_details;
				redraw(32);
				audio.play_tone(505, 12);
				return false;
			}
		}
	}

	return false;
}

widgetBattery widget_battery;