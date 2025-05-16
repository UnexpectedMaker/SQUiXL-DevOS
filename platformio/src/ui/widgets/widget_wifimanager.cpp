#include "ui/widgets/widget_wifimanager.h"
#include "ui/icons/images/ui_icons.h"
#include "ui/ui_screen.h"

void widgetWiFiManager::create()
{
	_x = 0;
	_y = 0;
	_c = TFT_WHITE;
	_w = 480;
	_h = 480;

	load_icons();
}

void widgetWiFiManager::load_icons()
{
	_sprite_content.createVirtual(_w, _h, NULL, true);
	_sprite_clean.createVirtual(_w, _h, NULL, true);
	_sprite_mixed.createVirtual(_w, _h, NULL, true);

	for (int i = 0; i < wifi_images_count; i++)
	{
		wifi_icons[i].createVirtual(32, 32, NULL, true);
		squixl.loadPNG_into(&wifi_icons[i], 0, 0, wifi_images[i], wifi_image_sizes[i]);
	}

	Serial.println("Loaded WiFi icons");
}

void widgetWiFiManager::capture_clean_sprite()
{
	squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
	is_dirty_hard = false;
}

bool widgetWiFiManager::redraw(uint8_t fade_amount, int8_t tab_group)
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

	if (is_dirty_hard)
	{
		_sprite_content.fillScreen(TFT_MAGENTA);
		_sprite_clean.fillScreen(TFT_MAGENTA);

		delay(10);
		is_dirty_hard = false;
	}

	_sprite_content.fillScreen(TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(TFT_WHITE, -1);
	_sprite_content.setCursor(40, 20);

	if (wifi_controller.is_connected())
	{
		_sprite_content.drawSprite(0, 0, &wifi_icons[4], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.print(WiFi.localIP());
	}
	else
	{
		_sprite_content.drawSprite(0, 0, &wifi_icons[0], 1.0f, 0x0, DRAW_TO_RAM);
		_sprite_content.print("Setup SQUiXL WiFi");
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, fade_amount);
		squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, 32);
		squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		next_refresh = millis();
	}

	is_dirty = false;
	is_busy = false;
	next_refresh = millis();

	return false;
}

bool widgetWiFiManager::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();

				if (back_screen != nullptr)
					squixl.set_current_screen(back_screen);
				audio.play_tone(300, 2);

				return true;
			}
		}
	}

	return false;
}

widgetWiFiManager widget_wifimanager;