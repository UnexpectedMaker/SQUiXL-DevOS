#include "ui/widgets/widget_mqtt_sensors.h"
#include "mqtt/mqtt.h"
#include "ui/ui_screen.h"

using json = nlohmann::json;

bool widgetMQTTSensors::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (mqtt_stuff.mqtt_dirty)
	{
		should_redraw = true;
		mqtt_stuff.mqtt_dirty = false;
	}

	if (fade_amount == 32 && should_redraw)
	{
		// squixl.lcd.drawSprite(_x, _y, &_sprite_clean, 1.0f, -1, DRAW_TO_LCD);
		is_dirty = true;
		should_redraw = false;
	}

	if (is_dirty_hard)
	{
		// This is used to re-capture a clean version of the background sprite for this UI element, so it can generate it's content over it.
		ui_parent->_sprite_back.readImage(_adj_x, _adj_y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());

		// If we have children, tell them to set their dirty hard state, so they re-capture the backgrounds
		if (ui_children.size() > 0)
		{
			for (int w = 0; w < ui_children.size(); w++)
			{
				if (ui_children[w] != nullptr)
					ui_children[w]->set_dirty_hard(true);
			}
		}
		is_dirty_hard = false;
	}

	if (is_dirty || fade_amount == 0)
	{
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		delay(10);

		draw_window_heading();

		// if (!test_sprite.getBuffer())
		// {
		// 	test_sprite.createVirtual(100, 100, NULL, true);
		// 	test_sprite_back.createVirtual(100, 100, NULL, true);
		// 	delay(10);
		// }

		// squixl.lcd.readImage(190, 190, 100, 100, (uint16_t *)test_sprite_back.getBuffer());
		// test_sprite.fillScreen(TFT_WHITE);
		// test_sprite.setFreeFont(UbuntuMono_B[4]);
		// test_sprite.setAntialias(true);
		// test_sprite.setTextColor(TFT_MAGENTA, TFT_WHITE);
		// test_sprite.setCursor(10, 60);
		// test_sprite.print("50%");

		// squixl.lcd.blendSprite(&test_sprite, &test_sprite_back, &test_sprite_back, 8, TFT_MAGENTA);
		// squixl.lcd.drawSprite(190, 190, &test_sprite_back, 1.0f, -1, DRAW_TO_LCD);

		for (auto &sensor_data : mqtt_stuff.mqtt_topic_payloads)
		{
			// Serial.printf("Parsing sensor data for %s\n", sensor_data.first.c_str());
			for (int s = 0; s < sensor_data.second.size(); s++)
			{
				if (sensor_data.second[s].device_class == "temperature" && sensor_data.second[s].dash_item == nullptr)
				{

					Serial.printf("Creating dash_item for %s\n", sensor_data.first.c_str());

					ui_gauge *dash_temp = new ui_gauge_bme280;

					add_child_ui(dash_temp);
					sensor_data.second[s].set_dash_item(dash_temp);
					dash_temp->create(sensor_x, sensor_y, 104, 104);
					dash_temp->set_payload_index(sensor_data.first.c_str(), s, sensor_data.second[s].device_class.c_str());
					dash_temp->fade(0.0, 1.0, 1000, true, true);

					sensor_x += 112;
					if (sensor_x > _w - 40) // width of the window + margins
					{
						sensor_x = 9;
						sensor_y += 112;
					}
				}
				else if (sensor_data.second[s].dash_item != nullptr)
				{
					sensor_data.second[s].dash_item->redraw(32);
				}
			}
		}
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, fade_amount);
		// squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else if (is_dirty)
	{

		// // Serial.printf("Doing window child thingo om %d children\n", children.size());
		// for (int w = 0; w < ui_children.size(); w++)
		// {
		// 	// ui_children[w]->set_dirty(true);
		// 	ui_children[w]->redraw(32);
		// }

		squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, 32);
		// squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, 0x0, DRAW_TO_LCD);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		// if (fade_amount < 32)
		// {
		// 	squixl.lcd.blendSprite(&_sprite_joke, &_sprite_back, &_sprite_mixed, fade_amount, TFT_MAGENTA);
		// 	squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		// }
		// else
		// {
		// 	squixl.lcd.blendSprite(&_sprite_joke, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);
		// 	squixl.current_screen()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);

		// 	Serial.println("Tick jokes");
		// }
		next_refresh = millis();
	}

	is_dirty = false;
	next_update = millis();

	return true;
}

bool widgetMQTTSensors::process_touch(touch_event_t touch_event)
{
	if (check_bounds(touch_event.x, touch_event.y))
	{
		for (int w = 0; w < ui_children.size(); w++)
		{
			touch_event_t adjusted_touch;
			adjusted_touch.x = touch_event.x - _x;
			adjusted_touch.y = touch_event.y - _y;
			adjusted_touch.type = touch_event.type;

			if (ui_children[w]->process_touch(adjusted_touch))
			{
				return true;
			}
		}

		if (touch_event.type == TOUCH_TAP)
		{
			Serial.println("TAP");
			audio.play_tone(300, 2);
			return true;
		}
		else if (touch_event.type == TOUCH_LONG)
		{
			Serial.println("LONG TAP");
			audio.play_tone(505, 2);
			return true;
		}
		else if (touch_event.type == TOUCH_DOUBLE)
		{
			Serial.println("Double TAP");
			audio.play_tone(800, 2);
			return true;
		}
	}
	return false;
}

widgetMQTTSensors widget_mqtt_sensors;
