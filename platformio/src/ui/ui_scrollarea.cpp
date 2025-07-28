#include "ui/ui_scrollarea.h"
#include "ui/ui_screen.h"
#include "mqtt/mqtt.h"

void ui_scrollarea::create(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, uint16_t color)
{
	_x = x;
	_y = y;
	_w = w;
	_h = h;
	_title = title;
	_c = color;

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);
}

void ui_scrollarea::slice_sprites()
{

	// draw the scroll area into a temp sprite
	_sprite_back.createVirtual(_w, _h, NULL, true);
	_sprite_back.setFreeFont(UbuntuMono_R[1]);
	_sprite_back.setTextColor(TFT_WHITE, -1);

	_sprite_back.fillScreen(TFT_MAGENTA);

	_sprite_back.setCursor(padding.left, char_height + 4);
	_sprite_back.print(_title.c_str());

	_sprite_back.fillRoundRect(0, 20, _w - 20, _h - 20, 6, static_cast<ui_screen *>(get_ui_parent())->dark_tint[2]);

	// Scrollbar mockup

	// Created sprites to slice visuals into
	_sprite_top.createVirtual(_w, 26, NULL, true);
	_sprite_bottom.createVirtual(_w, 26, NULL, true);
	_sprite_left.createVirtual(padding.left, _h - 46, NULL, true);
	_sprite_right.createVirtual(30, _h - 46, NULL, true);

	// Slice sprtes
	_sprite_back.readImage(0, 0, _w, 26, (uint16_t *)_sprite_top.getBuffer());
	_sprite_back.readImage(0, _h - 26, _w, 26, (uint16_t *)_sprite_bottom.getBuffer());
	_sprite_back.readImage(0, 26, padding.left, _h - 46, (uint16_t *)_sprite_left.getBuffer());
	_sprite_back.readImage(_w - 30, 26, 30, _h - 46, (uint16_t *)_sprite_right.getBuffer());

	content_sprite_width = _w - 30 - padding.left;
	content_sprite_height = _h - 46;

	_sprite_content.createVirtual(content_sprite_width, content_sprite_height, NULL, true);
	_sprite_content.fillScreen(static_cast<ui_screen *>(get_ui_parent())->dark_tint[2]);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(TFT_WHITE, -1);

	delay(10);
	_sprite_back.freeVirtual();
}

bool ui_scrollarea::redraw(uint8_t fade_amount, int8_t tab_group)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	unsigned long start_time = millis();

	if (!_sprite_top.getBuffer())
	{
		slice_sprites();
	}

	// Serial.printf("mqtt_stuff.mqtt_dirty %d, is_dragging %d, acceleration_y: %d, momentum: %d \n", mqtt_stuff.mqtt_dirty, is_dragging, acceleration_y, momentum);

	if (mqtt_stuff.mqtt_dirty || is_dragging || acceleration_y != 0)
	{
		_sprite_content.fillScreen(static_cast<ui_screen *>(get_ui_parent())->dark_tint[2]);

		//_scroll_y
		int line_y = char_height;
		content_height = 0;

		if (acceleration_y != 0 && !is_dragging)
		{
			acceleration_y = int(acceleration_y / 3);
			_scroll_y = constrain(_scroll_y + acceleration_y, scroll_y_min, 0);
			momentum = (acceleration_y == 0);
			// Serial.printf("accel_y: %d\n", acceleration_y);
		}

		// For now, this is just printing out the payload data on the content sprite so it can be displayed,
		// but this could of could be any type of graphics primative or groupd of primatives

		for (auto &sensor_data : mqtt_stuff.mqtt_topic_payloads)
		{
			for (int s = 0; s < sensor_data.second.size(); s++)
			{
				psram_string data = sensor_data.second[s].description + " " + sensor_data.second[s].device_class + " " + sensor_data.second[s].sensor_value + sensor_data.second[s].unit_of_measurement;
				_sprite_content.setCursor(10, _scroll_y + line_y);
				_sprite_content.print(data.c_str());
				line_y += 18;
				// This is nasty, but for now, easiest way to calculate the content height - used for clamping and scrollbar
				content_height += 18;
			}
		}

		content_size_perc = (float)content_sprite_height / (float)content_height;

		_sprite_right.fillRect(20, 4, 10, _h - 56, static_cast<ui_screen *>(get_ui_parent())->dark_tint[2]);

		if (content_size_perc > 1.0f)
		{
			// content is shorter than content area height
			_sprite_right.fillRect(20, 4, 10, _h - 56, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1]);
		}
		else
		{
			// Content is longer than content area height, so adjust length of scrollbar indicator
			// still have to also calc the visual position of the bar, but this is enough for now
			int16_t scroll_track_length = _h - 56;
			int16_t len = scroll_track_length * content_size_perc;
			scroll_y_min = (content_sprite_height - content_height);

			int16_t thumb_movable_range = scroll_track_length - len;
			// Percentage scrolled (0.0f at top, 1.0f at bottom)
			float scroll_perc = (float)_scroll_y / (float)(content_sprite_height - content_height);
			if (scroll_perc < 0.0f)
				scroll_perc = 0.0f;
			if (scroll_perc > 1.0f)
				scroll_perc = 1.0f;

			// Calculate thumb position
			int16_t scroll_y_pos = (int16_t)(scroll_perc * thumb_movable_range);

			// Serial.printf("scroll_y_pos: %d, thumb_movable_range: %d, scroll_perc: %0.2f\n", scroll_y_pos, thumb_movable_range, scroll_perc);

			_sprite_right.fillRect(20, 4 + scroll_y_pos, 10, len, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1]);
		}

		if (mqtt_stuff.mqtt_dirty)
			mqtt_stuff.mqtt_dirty = false;
	}
	else
	{
		momentum = false;
		is_dragging = false;
		// Serial.println("ping");
	}

	// draw the content and scrollbar
	if (get_ui_parent()->_sprite_content.getBuffer())
	{
		get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_top, 1.0f, -1, DRAW_TO_RAM);
		get_ui_parent()->_sprite_content.drawSprite(_x, _y + _h - 26, &_sprite_bottom, 1.0f, -1, DRAW_TO_RAM);
		get_ui_parent()->_sprite_content.drawSprite(_x, _y + 26, &_sprite_left, 1.0f, -1, DRAW_TO_RAM);
		get_ui_parent()->_sprite_content.drawSprite(_x + _w - 30, _y + 26, &_sprite_right, 1.0f, -1, DRAW_TO_RAM);
		get_ui_parent()->_sprite_content.drawSprite(_x + padding.left, _y + 26, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
	}

	// Serial.printf("scroll redraw time: %u ms\n", (millis() - start_time));

	if (is_dragging || acceleration_y != 0)
	{
		// We are draggins so force refresh the screen
		// get_ui_parent()->redraw(32);
		next_refresh = millis() + refresh_interval;
	}
	else
	{

		next_refresh = millis();
	}

	is_dirty = false;
	is_busy = false;
	content_changed = false;

	return true;
}

bool ui_scrollarea::process_touch(touch_event_t touch_event)
{
	// Serial.printf("event? %d, dragging? %d\n", touch_event.type, is_dragging);
	if (touch_event.type == TOUCH_DRAG_END)
	{
		// Serial.printf("Drag End: d_x: %d, d_y: %d\n", touch_event.d_x, touch_event.d_y);
		is_dragging = false;

		// acceleration_x = _cached_scroll_x;
		// acceleration_y = _cached_scroll_y;
		return true;
	}

	if (!is_dragging && !momentum)
	{
		if (touch_event.type == SCREEN_DRAG_H && drag_able != DRAGGABLE::DRAG_HORIZONTAL)
			return false;

		if (touch_event.type == SCREEN_DRAG_V && drag_able != DRAGGABLE::DRAG_VERTICAL)
			return false;
	}

	// Serial.printf("1: d_x: %d, d_y: %d\n", touch_event.d_x, touch_event.d_y);

	if (content_size_perc > 1.0f)
		return false;

	if (!momentum)
		is_dragging = true;

	if (drag_able == DRAGGABLE::DRAG_VERTICAL)
	{
		_scroll_x = 0;
		_scroll_y = constrain(_scroll_y + touch_event.d_y, scroll_y_min, 0);
		// Serial.println("y");
	}
	else if (drag_able == DRAGGABLE::DRAG_HORIZONTAL)
	{
		_scroll_x = constrain(_scroll_x + touch_event.d_x, scroll_x_min, 0);
		_scroll_y = 0;
		// Serial.println("x");
	}
	else
	{
		return false;
	}

	if (_scroll_x != _cached_scroll_x || _scroll_y != _cached_scroll_y)
	{
		// Serial.printf("scroll %d and %d\n", _scroll_x, _scroll_y);
		content_changed = true;
		redraw(32);
	}
	_cached_scroll_x = _scroll_x;
	_cached_scroll_y = _scroll_y;

	acceleration_y = touch_event.d_y;
	acceleration_x = touch_event.d_x;

	// Serial.printf("set accel_y to %d\n", acceleration_y);

	return true;
}

void ui_scrollarea::about_to_show_screen()
{
	is_dragging = false;
	acceleration_y = 0;
	// Serial.printf("show! dragging? %d\n", is_dragging);
}

void ui_scrollarea::about_to_close_screen()
{
	is_dragging = false;
	acceleration_y = 0;
	// Serial.printf("close! dragging? %d\n", is_dragging);
}