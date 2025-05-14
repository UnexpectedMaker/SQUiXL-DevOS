#include "ui/ui_screen.h"
#include "ui/ui_keyboard.h"
#include "settings/settings.h"
#include "JPEGDisplay.inl"
// #include "PNGDisplay.inl"

uint16_t background_colors[6] = {0x5AEB, darken565(0x5AEB, 0.5), 0x001f, 0xf800, darken565(0x07e0, 0.5), 0xbbc0};

void ui_screen::setup(uint16_t _back_color, bool add)
{
	_x = 0;
	_y = 0;
	_w = 480;
	_h = 480;

	back_color = _back_color;

	_sprite_clean.createVirtual(480, 480, NULL, true);
	_sprite_clean.fillScreen(back_color);

	_sprite_back.createVirtual(480, 480, NULL, true);
	_sprite_back.fillScreen(back_color);

	_sprite_content.createVirtual(480, 480, NULL, true);
	_sprite_content.fillScreen(TFT_MAGENTA);

	set_refresh_interval(20);

	calc_new_tints();

	// squixl.add_screen(this);
}

void ui_screen::calc_new_tints()
{
	float c = 0.1f;
	for (int i = 0; i < 8; i++)
	{
		dark_tint[i] = darken565(back_color, c);
		light_tint[i] = lighten565(back_color, c);
		c += 0.1;
	}
}

void ui_screen::set_navigation(Directions from, ui_screen *screen, bool set_reversed)
{
	if ((int)from < 4)
	{
		navigation[(int)from] = screen;

		if (set_reversed)
		{
			int alt_dir = ((int)from - 2) & 3;
			if (screen != nullptr)
				screen->set_navigation((Directions)alt_dir, this);
		}
	}
}

void ui_screen::show_background_jpg(const void *jpg, int jpg_size, bool fade_in)
{
	is_busy = true;

	int w, h, bpp;

	bool has_content = false;
	if (background_size == 0)
	{
		_sprite_clean.fillScreen(TFT_BLACK);
	}
	else
	{
		squixl.lcd.readImage(0, 0, 480, 480, (uint16_t *)_sprite_clean.getBuffer());
		delay(5);
		has_content = true;
	}

	background_size = jpg_size;

	if (squixl.jd.getJPEGInfo(&w, &h, &bpp, jpg, jpg_size))
	{
		// Serial.printf("JPG info: w %d, h %d, bpp %d\n", w, h, bpp);

		squixl.jd.loadJPEG(&_sprite_back, 0, 0, jpg, jpg_size);

		if (fade_in)
		{
			if (has_content)
			{
				// we only need this sprite temporarly if we are blending content
				if (!_sprite_mixed.getBuffer())
					_sprite_mixed.createVirtual(480, 480, NULL, true);
			}

			for (uint8_t u8Alpha = 0; u8Alpha < 32; u8Alpha += 4)
			{
				if (has_content)
				{
					squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &_sprite_mixed, u8Alpha);
					squixl.lcd.blendSprite(&_sprite_content, &_sprite_mixed, &squixl.lcd, 32, TFT_MAGENTA);
				}
				else
				{
					squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &squixl.lcd, u8Alpha);
				}
			}

			if (_sprite_mixed.getBuffer())
				_sprite_mixed.freeVirtual();
		}
		else
		{
			squixl.lcd.drawSprite(0, 0, &_sprite_back, 1.0f, -1, DRAW_TO_LCD);
		}

		Serial.println("Screen Wallpaper loaded and displayed... loading children UI");
	}
	else
	{
		Serial.printf("JPG not loaded - size was %d :(\n", jpg_size);
	}

	for (int w = 0; w < ui_children.size(); w++)
	{
		if (ui_children[w] != nullptr)
		{
			ui_children[w]->set_dirty_hard(true);
			ui_children[w]->redraw(32);
		}
	}

	if (!has_content && fade_in)
	{
		for (uint8_t u8Alpha = 0; u8Alpha <= 32; u8Alpha += 2)
		{
			redraw(u8Alpha);
			delay(2);
		}
	}
	else
	{
		redraw(32);
	}

	is_busy = false;
	next_refresh = millis();
}

void ui_screen::show_next_background()
{
	uint8_t index = squixl.cycle_next_wallpaper();
	if (index == 0)
		show_background_jpg(wallpaper_01, sizeof(wallpaper_01));
	else if (index == 1)
		show_background_jpg(wallpaper_02, sizeof(wallpaper_02));
	else if (index == 2)
		show_background_jpg(wallpaper_03, sizeof(wallpaper_03));
	else if (index == 3)
		show_background_jpg(wallpaper_04, sizeof(wallpaper_04));
	else if (index == 4)
		show_background_jpg(wallpaper_05, sizeof(wallpaper_05));
	else
		show_background_jpg(wallpaper_06, sizeof(wallpaper_06));
}

void ui_screen::show_random_background(bool fade)
{
	// Pick a random wallpaper
	uint8_t index = random(0, 6);
	// Set the background as the current one for save data and cycling
	squixl.set_wallpaper_index(index);

	if (index == 0)
		show_background_jpg(wallpaper_01, sizeof(wallpaper_01), fade);
	else if (index == 1)
		show_background_jpg(wallpaper_02, sizeof(wallpaper_02), fade);
	else if (index == 2)
		show_background_jpg(wallpaper_03, sizeof(wallpaper_03), fade);
	else if (index == 3)
		show_background_jpg(wallpaper_04, sizeof(wallpaper_04), fade);
	else if (index == 4)
		show_background_jpg(wallpaper_05, sizeof(wallpaper_05), fade);
	else
		show_background_jpg(wallpaper_06, sizeof(wallpaper_06), fade);
}

void ui_screen::refresh(bool forced)
{
	unsigned long start_time = millis();

	if (is_busy)
	{
		Serial.println("BUSY!!!!");
		return;
	}

	if (keyboard.showing)
	{
		// Serial.println("KB open!!!!");
		// keyboard.update();
		next_refresh = millis();
		return;
	}

	// if (next_refresh == 0)
	// {
	// 	set_refresh_interval(3);
	// 	// forced = true;
	// }

	bool child_dirty = false;

	for (int w = 0; w < ui_children.size(); w++)
	{
		if (ui_children[w] != nullptr)
		{
			if (ui_children[w]->should_refresh())
			{
				if (ui_children[w]->redraw(32))
					child_dirty = true;
			}
		}
	}

	if (child_dirty || animation_manager.active_animations() > 0 || forced)
	{
		if (animation_manager.active_animations() > 0)
			refresh_interval = 1;
		else
			refresh_interval = 20;
		// Serial.printf("screen update - child_dirty? %d, forced? %d, anims? %d\n", child_dirty, forced, animation_manager.active_animations());
		redraw(32);
	}

	next_refresh = millis();
	// Serial.printf("refresh time: %u ms, overlay_alpha: %u\n", (millis() - start_time), overlay_alpha);
	// Serial.printf("Nex screen refresh is at %u\n", next_refresh + refresh_interval);
}

bool ui_screen::process_touch(touch_event_t touch_event)
{

	// if (touch_event.type == TOUCH_TAP)
	// {
	// 	if (check_bounds(touch_event.x, touch_event.y))
	// 	{
	// 		if (millis() - next_click_update > 1000)
	// 		{
	// 			next_click_update = millis();
	// 			Serial.printf("TAP at %d, %d\n", touch_event.x, touch_event.y);
	// 			audio.play_tone(300, 2);

	// 			return true;
	// 		}
	// 	}
	// }

	// return true;

	// Did any of my children recieve this touch event?
	for (int w = 0; w < ui_children.size(); w++)
	{
		if (ui_children[w]->process_touch(touch_event))
		{
			next_update = 0;
			return true;
		}
	}

	if (touch_event.type == TOUCH_LONG)
	{
		if (can_cycle_background_color)
		{
			audio.play_tone(1500, 1);
			back_color = background_colors[random(5)];
			_sprite_back.fillScreen(back_color);
			calc_new_tints();
			for (int w = 0; w < ui_children.size(); w++)
			{
				if (ui_children[w] != nullptr)
					ui_children[w]->redraw(32);
			}
			// delay(10);
			redraw(32);
			return true;
		}

		return false;
	}
	else if (int(touch_event.type) >= 3 && int(touch_event.type) < 7)
	{
		String sw_dir[4] = {"SWIPE UP", "SWIPE RIGHT", "SWIPE DOWN", "SWIPE LEFT"};

		bool swipe_ok = false;
		// get the direction from the wipe dir
		int nav_dir = int(touch_event.type) - 3;

		// Serial.printf("touch event: %d, (%d, %d) sw_dir: %s, nav_dir: %d\n", int(touch_event.type), touch_event.x, touch_event.y, sw_dir[int(touch_event.type) - 3], nav_dir);

		if (navigation[nav_dir] != nullptr)
		{
			squixl.set_current_screen(navigation[nav_dir]);
			swipe_ok = true;

			squixl.current_screen()->animate_pos((Directions)nav_dir, 250, tween_ease_t::EASE_OUT, nullptr);
		}

		if (swipe_ok)
		{
			// Serial.printf("New screen index: %d\n", settings.config.current_screen);
			return true;
		}
	}

	return false;
}

bool ui_screen::redraw(uint8_t fade_amount)
{

	// // Have the main screen sprites been setup?
	// if (!_sprite_clean.getBuffer())
	// {
	// 	setup();
	// }

	unsigned long start_time = millis();

	if (fade_amount < 32)
	{
		if (blend_transparency)
			squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, fade_amount, TFT_MAGENTA);
		else
			squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, fade_amount);
	}
	else
	{
		if (squixl.switching_screens)
		{
			// squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);
			squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		}
		else
		{
			if (blend_transparency)
				squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, 32, TFT_MAGENTA);
			else
				squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, 32);

			rebuild_mixed_sprite = true;
		}

		next_refresh = millis();
	}

	is_dirty = false;

	// Serial.printf("redraw time: %u ms\n", (millis() - start_time));
	return true;
};

// Animation

void ui_screen::fade_overlay(uint8_t fade_amount)
{
	overlay_alpha = fade_amount;
	redraw(32);
}

void ui_screen::show_overlay(bool show, unsigned long duration, std::function<void()> completion_callback)
{
	float from = show ? 0.0 : 1.0;
	float to = show ? 1.0 : 0.0;

	animation_manager.add_animation(new tween_animation(from, to, duration, tween_ease_t::EASE_LINEAR, [this](float opacity) {
			uint8_t fade_amount = int(32.0 * opacity);
			this->fade_overlay(fade_amount); }, [completion_callback]() {
				if (completion_callback) {
					completion_callback();
				} }));
}

void ui_screen::animate_pos(Directions direction, unsigned long duration, tween_ease_t ease, std::function<void()> completion_callback)
{
	float from_x = 0.0;
	float to_x = 0.0;
	float from_y = 0.0;
	float to_y = 0.0;
	switch (direction)
	{
	case LEFT:
		from_x = 480.0;
		break;
	case RIGHT:
		from_x = -480.0;
		break;
	case UP:
		from_y = 480.0;
		break;
	case DOWN:
		from_y = -480.0;
		break;
	case UL:
		from_x = 480.0;
		from_y = 480.0;
		break;
	case DR:
		from_x = -480.0;
		from_y = -480.0;
		to_x = 0.0;
		to_y = 0.0;
		break;
	}

	squixl.switching_screens = true;

	unsigned long start_time = millis();
	float t = 0.0;

	// Initially force the screen to the start position and make it redarw to ensure it's got contents on it if it's the first time viewing it.
	// _x = (int)from_x;
	// _y = (int)from_y;

	for (int w = 0; w < ui_children.size(); w++)
	{
		if (ui_children[w] != nullptr)
		{
			ui_children[w]->redraw(32);
		}
	}

	// we only need this sprite temporarly if we are blending content
	if (!_sprite_mixed.getBuffer())
		_sprite_mixed.createVirtual(480, 480, NULL, true);

	squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);

	while (t < 1.0 || _x != (int)to_x || _y != (int)to_y)
	{
		unsigned long now = millis();
		t = (now - start_time) / (float)duration;
		if (t >= 1.0f)
		{
			t = 1.0f;
		}

		// Choose your easing function based on an 'ease' member.
		float eased_t;
		switch (ease)
		{
		case EASE_LINEAR:
			eased_t = t;
			break;
		case EASE_IN:
			eased_t = t * t;
			break;
		case EASE_OUT:
			eased_t = t * (2 - t);
			break;
		case EASE_IN_OUT:
			eased_t = (t < 0.5f) ? (2 * t * t) : (-1 + (4 - 2 * t) * t);
			break;
		default:
			eased_t = t;
			break;
		}

		float current_x = from_x + (to_x - from_x) * eased_t;
		float current_y = from_y + (to_y - from_y) * eased_t;

		_x = (int)current_x;
		_y = (int)current_y;
		redraw(32);

		// Serial.printf("(%d, %d) @ %f\n", _x, _y, t);
		delay(5);
	}

	// Clear this buffer
	if (_sprite_mixed.getBuffer())
		_sprite_mixed.freeVirtual();

	squixl.switching_screens = false;
	// Serial.println("Anim complete");
}
