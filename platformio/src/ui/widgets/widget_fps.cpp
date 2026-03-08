#include "ui/widgets/widget_fps.h"
#include "ui/ui_screen.h"

void widgetFPS::create(int16_t x, int16_t y, uint16_t color)
{
	_x = x;
	_y = y;
	_c = color;
	_w = 80;
	_h = 20;
}

void widgetFPS::tick()
{
	if (last_fps_time == 0)
	{
		last_fps_time = millis();
		return;
	}

	loop_count++;
	unsigned long now = millis();
	if (now - last_fps_time >= 1000)
	{
		fps = loop_count;
		loop_count = 0;
		last_fps_time = now;
		is_dirty = true;
	}
}

bool widgetFPS::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (millis() < delay_first_draw)
		return false;

	if (is_busy)
		return false;

	is_busy = true;

	if (!sprite_created)
	{
		sprite_created = true;
		_sprite_content.create(_w, _h);
		last_fps_time = millis();
	}

	_sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);

	_sprite_content.setFreeFont(UbuntuMono_R[1]);
	_sprite_content.setTextColor(_c, TFT_MAGENTA);
	_sprite_content.setCursor(0, 13);
	_sprite_content.printf("FPS: %lu", fps);

	ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1);

	next_refresh = millis();
	is_dirty = false;
	is_busy = false;

	return false;
}
