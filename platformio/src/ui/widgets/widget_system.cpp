#include "ui/widgets/widget_system.h"
#include "ui/icons/images/ui_icons.h"
#include "web/wifi_controller.h"

#include "ui/ui_screen.h"

void widgetSystem::create(int16_t x, int16_t y, uint16_t color)
{
	_x = x;
	_y = y;
	_c = color;
	_w = 0;
	_h = 0;

	touch_padding = 5;
}

void widgetSystem::load_icons()
{
	// Create the required sprites
	_w = 32;
	_h = 96;
	_sprite_content.createVirtual(_w, _h, NULL, true);
	_sprite_clean.createVirtual(_w, _h, NULL, true);
	_sprite_mixed.createVirtual(_w, _h, NULL, true);

	// Load icons
	icon_system.createVirtual(32, 32, NULL, true);
	squixl.loadPNG_into(&icon_system, 0, 0, ui_bolt, sizeof(ui_bolt));

	icons_volume[0].createVirtual(32, 32, NULL, true);
	squixl.loadPNG_into(&icons_volume[0], 0, 0, volume_images[0], volume_image_sizes[0]);
	icons_volume[1].createVirtual(32, 32, NULL, true);
	squixl.loadPNG_into(&icons_volume[1], 0, 0, volume_images[1], volume_image_sizes[1]);
	icons_volume[2].createVirtual(32, 32, NULL, true);
	squixl.loadPNG_into(&icons_volume[2], 0, 0, volume_images[2], volume_image_sizes[2]);
	icons_volume[3].createVirtual(32, 32, NULL, true);
	squixl.loadPNG_into(&icons_volume[3], 0, 0, volume_images[3], volume_image_sizes[3]);

	Serial.println("Loaded icons");
}

void widgetSystem::capture_clean_sprite()
{
	squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
	is_dirty_hard = false;
}

bool widgetSystem::redraw(uint8_t fade_amount, int8_t tab_group)
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

		// delay(10);

		_sprite_content.fillScreen(TFT_MAGENTA);

		_sprite_content.drawSprite(0, 0, &icons_volume[3], 1.0f, 0x0, DRAW_TO_RAM);

		// Buttom right is the system icon
		_sprite_content.drawSprite(0, 48, &icon_system, 1.0f, 0x0, DRAW_TO_RAM);

		squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, 32);
		squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

		is_dirty_hard = false;

		Serial.println("Tick system");
	}

	if (fade_amount == 32)
		next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	return true;
}

bool widgetSystem::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (millis() - next_click_update > 500)
			{
				next_click_update = millis();
				// squixl.toggle_settings();
				// Serial.println("TAP");
				audio.play_tone(300, 2);

				return true;
			}
		}
	}

	return false;
}

widgetSystem widget_system;