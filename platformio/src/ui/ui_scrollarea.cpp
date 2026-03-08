#include "ui/ui_scrollarea.h"

#include "Arduino.h"
#include "ui/ui_screen.h"

#include <algorithm>

void ui_scrollarea::create(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, uint16_t color)
{
	_x = x;
	_y = y;
	_w = w;
	_h = h;
	_title = title ? title : "";
	_c = color;

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);
	content_changed = true;
}

void ui_scrollarea::slice_sprites()
{
	_sprite_back.create(_w, _h, TFT_MAGENTA);
	_sprite_back.setFreeFont(UbuntuMono_R[1]);
	_sprite_back.setTextColor(TFT_WHITE, -1);

	// _sprite_back.fillScreen(TFT_MAGENTA);

	_sprite_back.setCursor(padding.left, char_height + 2);
	_sprite_back.print(_title.c_str());

	auto *screen = static_cast<ui_screen *>(get_ui_parent());
	body_color = screen ? screen->dark_tint[2] : TFT_BLACK;

	_sprite_back.fillRoundRect(0, 20, _w - 20, _h - 20, 6, body_color);

	_sprite_top.create(_w, 26);
	_sprite_bottom.create(_w, 26);
	_sprite_left.create(padding.left, _h - 46);
	_sprite_right.create(30, _h - 46);

	_sprite_back.readImage(0, 0, _w, 26, (uint16_t *)_sprite_top.getBuffer());
	_sprite_back.readImage(0, _h - 26, _w, 26, (uint16_t *)_sprite_bottom.getBuffer());
	_sprite_back.readImage(0, 26, padding.left, _h - 46, (uint16_t *)_sprite_left.getBuffer());
	_sprite_back.readImage(_w - 30, 26, 30, _h - 46, (uint16_t *)_sprite_right.getBuffer());

	content_sprite_width = _w - 30 - padding.left;
	content_sprite_height = _h - 46;

	_sprite_content.create(content_sprite_width, content_sprite_height, body_color);
	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(TFT_WHITE, -1);
	// _sprite_content.fillScreen(body_color);

	delay(10);
	_sprite_back.release();
}

bool ui_scrollarea::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (tab_group >= 0 && !check_tab_group(tab_group))
	{
		return false;
	}

	if (is_busy)
	{
		return false;
	}
	is_busy = true;

	if (!_sprite_top.getBuffer())
	{
		slice_sprites();
	}

	bool needs_content = content_changed || external_content_dirty() || is_dragging || acceleration_y != 0;

	if (needs_content)
	{
		if (acceleration_y != 0 && !is_dragging)
		{
			acceleration_y = int(acceleration_y / 3);
			_scroll_y = constrain(_scroll_y + acceleration_y, scroll_y_min, 0);
			momentum = (acceleration_y == 0);
		}

		if (acceleration_x != 0 && !is_dragging)
		{
			acceleration_x = int(acceleration_x / 3);
			_scroll_x = constrain(_scroll_x + acceleration_x, scroll_x_min, 0);
		}

		_sprite_content.fillScreen(content_background_color());
		_sprite_content.setFreeFont(UbuntuMono_R[1]);
		_sprite_content.setTextColor(TFT_WHITE, -1);

		content_height = 0;
		render_content();

		update_scroll_metrics();

		after_content_render();

		content_changed = false;
	}
	else
	{
		momentum = false;
		is_dragging = false;
	}

	if (get_ui_parent() && get_ui_parent()->_sprite_content.getBuffer())
	{
		get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_top, 1.0f, -1);
		get_ui_parent()->_sprite_content.drawSprite(_x, _y + _h - 26, &_sprite_bottom, 1.0f, -1);
		get_ui_parent()->_sprite_content.drawSprite(_x, _y + 26, &_sprite_left, 1.0f, -1);
		get_ui_parent()->_sprite_content.drawSprite(_x + _w - 30, _y + 26, &_sprite_right, 1.0f, -1);
		get_ui_parent()->_sprite_content.drawSprite(_x + padding.left, _y + 26, &_sprite_content, 1.0f, -1);
	}

	next_refresh = (is_dragging || acceleration_y != 0) ? millis() + refresh_interval : millis();

	is_dirty = false;
	is_busy = false;
	return true;
}

bool ui_scrollarea::process_touch(touch_event_t touch_event)
{
	if (touch_event.x < _x || touch_event.x > _x + _w || touch_event.y < _y || touch_event.y > _y + _h)
	{
		return false;
	}

	if (touch_event.type == TOUCH_DRAG_END)
	{
		is_dragging = false;
		return true;
	}

	if (!is_dragging && !momentum)
	{
		if (touch_event.type == SCREEN_DRAG_H && drag_able != DRAGGABLE::DRAG_HORIZONTAL)
			return false;

		if (touch_event.type == SCREEN_DRAG_V && drag_able != DRAGGABLE::DRAG_VERTICAL)
			return false;
	}

	if (content_size_perc > 1.0f)
		return false;

	if (!momentum)
		is_dragging = true;

	if (drag_able == DRAGGABLE::DRAG_VERTICAL)
	{
		_scroll_x = 0;
		_scroll_y = constrain(_scroll_y + touch_event.d_y, scroll_y_min, 0);
	}
	else if (drag_able == DRAGGABLE::DRAG_HORIZONTAL)
	{
		_scroll_x = constrain(_scroll_x + touch_event.d_x, scroll_x_min, 0);
		_scroll_y = 0;
	}
	else
	{
		return false;
	}

	if (_scroll_x != _cached_scroll_x || _scroll_y != _cached_scroll_y)
	{
		content_changed = true;
		redraw(32);
	}

	_cached_scroll_x = _scroll_x;
	_cached_scroll_y = _scroll_y;

	acceleration_y = touch_event.d_y;
	acceleration_x = touch_event.d_x;

	return true;
}

void ui_scrollarea::about_to_show_screen()
{
	is_dragging = false;
	acceleration_y = 0;
}

void ui_scrollarea::about_to_close_screen()
{
	is_dragging = false;
	acceleration_y = 0;
}

uint16_t ui_scrollarea::content_background_color()
{
	auto *screen = static_cast<ui_screen *>(get_ui_parent());
	return screen ? screen->dark_tint[2] : TFT_BLACK;
}

uint16_t ui_scrollarea::scroll_track_color()
{
	return content_background_color();
}

uint16_t ui_scrollarea::scroll_thumb_color()
{
	auto *screen = static_cast<ui_screen *>(get_ui_parent());
	return screen ? screen->dark_tint[1] : TFT_WHITE;
}

void ui_scrollarea::update_scroll_metrics()
{
	_sprite_right.fillRect(20, 4, 10, _h - 56, scroll_track_color());

	if (content_height <= 0)
	{
		content_height = content_sprite_height;
	}

	if (content_height <= 0 || content_sprite_height <= 0)
	{
		content_size_perc = 1.0f;
		scroll_y_min = 0;
		_sprite_right.fillRect(20, 4, 10, _h - 56, scroll_thumb_color());
		return;
	}

	content_size_perc = static_cast<float>(content_sprite_height) / static_cast<float>(content_height);
	scroll_y_min = content_sprite_height - content_height;

	if (content_size_perc >= 1.0f)
	{
		content_size_perc = 1.0f;
		scroll_y_min = 0;
		_sprite_right.fillRect(20, 4, 10, _h - 56, scroll_thumb_color());
		return;
	}

	int16_t scroll_track_length = _h - 56;
	int16_t len = std::max<int16_t>(4, static_cast<int16_t>(scroll_track_length * content_size_perc));
	int16_t thumb_movable_range = scroll_track_length - len;

	float denom = static_cast<float>(content_sprite_height - content_height);
	float scroll_perc = denom == 0.0f ? 0.0f : static_cast<float>(_scroll_y) / denom;
	scroll_perc = std::max(0.0f, std::min(1.0f, scroll_perc));

	int16_t scroll_y_pos = static_cast<int16_t>(scroll_perc * thumb_movable_range);

	_sprite_right.fillRect(20, 4 + scroll_y_pos, 10, len, scroll_thumb_color());
}
