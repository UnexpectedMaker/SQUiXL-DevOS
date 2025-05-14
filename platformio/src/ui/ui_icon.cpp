#include "ui/ui_icon.h"
#include "ui/icons/images/ui_icons.h"

void ui_icon::create(int16_t x, int16_t y, int16_t target_w, int16_t target_h, const void *image, int image_size, bool fade_in)
{
	_x = x;
	_y = y;
	_w = target_w;
	_h = target_h;
	_target_w = target_w;
	_target_h = target_h;

	_icon.createVirtual(_target_w, _target_h, NULL, true);
	_sprite_content.createVirtual(_target_w, _target_h, NULL, true);
	_sprite_mixed.createVirtual(_target_w, _target_h, NULL, true);
	_sprite_clean.createVirtual(_target_w, _target_h, NULL, true);
	delay(10);

	squixl.loadPNG_into(&_icon, 0, 0, ui_bolt, sizeof(ui_bolt));
}

bool ui_icon::redraw(uint8_t fade_amount)
{

	if (fade_amount == 0)
	{
		// Serial.println("Skipping redraw due to 0 fade value");
		return false;
	}

	if (is_dirty_hard)
	{
		_sprite_content.fillScreen(TFT_MAGENTA);
		_sprite_clean.fillScreen(TFT_MAGENTA);
		_sprite_content.drawSprite(0, 0, &_icon, 1.0f, 0x0, DRAW_TO_RAM);

		delay(10);
		is_dirty_hard = false;
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