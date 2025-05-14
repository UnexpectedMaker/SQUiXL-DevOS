#include "ui/dashboard/ui_gauge_bme280.h"
#include "mqtt/mqtt.h"

bool ui_gauge_bme280::redraw(uint8_t fade_amount)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{
		squixl.lcd.readImage(ui_parent->pos_x() + _x, ui_parent->pos_y() + _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		// squixl.lcd.readImage(ui_parent->pos_x() + _x, ui_parent->pos_y() + _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		squixl.lcd.readImage(ui_parent->pos_x() + _x, ui_parent->pos_y() + _y, _w, _h, (uint16_t *)_sprite_content.getBuffer());
		is_dirty_hard = false;
		is_aniamted_cached = false;
	}

	MQTT_Payload _payload = mqtt_stuff.mqtt_topic_payloads[_payload_key][_payload_index];

	if (is_aniamted_cached && _payload.is_dirty)
		is_aniamted_cached = false;

	if (!is_aniamted_cached)
	{
		// Add the tinted background
		_sprite_content.fillRoundRect(0, 0, _w, _h, 9, TFT_WHITE, DRAW_TO_RAM);
		squixl.lcd.blendSprite(&_sprite_content, &_sprite_clean, &_sprite_back, 16);

		// _sprite_back.drawSprite(0, 0, &_graphic_back, 1.0, 0x0, DRAW_TO_RAM);
		// _sprite_back.drawSprite(0, 0, &_graphic_front, 1.0, 0x0, DRAW_TO_RAM);

		int text_w;
		int text_h;

		calc_text_size(_payload.get_sensor_value().c_str(), UbuntuMono_B[1], &text_w, &text_h);

		_sprite_back.setFreeFont(UbuntuMono_B[1]);
		_sprite_back.setCursor(_w / 2 - (text_w / 2), _h / 2 + text_h / 2 + 12);
		_sprite_back.setTextColor(TFT_BLUE, -1);
		_sprite_back.print(_payload.get_sensor_value().c_str());

		if (cached_device_text_w == 0 || cached_device_text_h == 0)
			calc_text_size(_payload.device_class.c_str(), UbuntuMono_R[1], &cached_device_text_w, &cached_device_text_h);

		_sprite_back.setFreeFont(UbuntuMono_R[1]);
		_sprite_back.setTextColor(TFT_CYAN, -1);
		_sprite_back.setCursor(_w / 2 - (cached_device_text_w / 2), _h - 5 - cached_device_text_h);
		_sprite_back.print(_payload.device_class.c_str());

		if (cached_desc_text_w == 0 || cached_desc_text_h == 0)
			calc_text_size(_payload.description.c_str(), UbuntuMono_R[0], &cached_desc_text_w, &cached_desc_text_h);

		_sprite_back.setFreeFont(UbuntuMono_R[0]);
		_sprite_back.setTextColor(TFT_BLACK, -1);
		_sprite_back.setCursor(_w / 2 - (cached_desc_text_w / 2), _h - 5);
		_sprite_back.print(_payload.description.c_str());

		_payload.is_dirty = false;
		is_aniamted_cached = true;
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, fade_amount);
		squixl.lcd.drawSprite(ui_parent->pos_x() + _x, ui_parent->pos_y() + _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, 32);
		squixl.lcd.drawSprite(ui_parent->pos_x() + _x, ui_parent->pos_y() + _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
		next_refresh = millis();
	}

	is_busy = false;
	return true;
}

bool ui_gauge_bme280::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		// Serial.printf("Tap: %d,%d Bounds: %d, %d - %d, %d\n", touch_event.x, touch_event.y, _x, _y, _w, _h);
		if (check_bounds(touch_event.x, touch_event.y))
		{
			// if (is_busy)
			// 	return false;

			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();

				_payload_index++;
				if (_payload_index >= mqtt_stuff.mqtt_topic_payloads[_payload_key].size())
					_payload_index = 0;

				Serial.printf("Sensor index is now %d\n", _payload_index);

				audio.play_tone(505, 12);
				redraw(32);
				return true;
			}
		}
	}

	return false;
}
