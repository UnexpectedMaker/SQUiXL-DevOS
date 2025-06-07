#include "ui/ui_screen.h"
#include "ui/ui_keyboard.h"
#include "ui/ui_dialogbox.h"
#include "settings/settings_async.h"
#include "JPEGDisplay.inl"

uint16_t background_colors[6] = {0x5AEB, darken565(0x5AEB, 0.5), 0x001f, 0xf800, darken565(0x07e0, 0.5), 0xbbc0};

void ui_screen::set_page_tabgroup(ui_control_tabgroup *tabgroup)
{
	ui_tab_group = tabgroup;
	tabgroup->ui_parent = this;
}

int8_t ui_screen::get_tab_group_index()
{
	if (ui_tab_group == nullptr)
		return -1;

	return ui_tab_group->get_current_tab();
}

void ui_screen::setup(uint16_t _back_color, bool add)
{
	_x = 0;
	_y = 0;
	_w = 480;
	_h = 480;

	back_color = _back_color;

	set_refresh_interval(20);

	calc_new_tints();

	set_draggable(DRAGABLE::DRAG_BOTH);
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

void ui_screen::create_buffers()
{
	if (!_sprite_back.getBuffer())
	{
		_sprite_back.createVirtual(480, 480, NULL, true);
		// _sprite_back.fillScreen(back_color);
		_sprite_back.fillRect(0, 0, 480, 480, back_color);
	}

	if (!_sprite_content.getBuffer())
	{
		_sprite_content.createVirtual(480, 480, NULL, true);
		_sprite_content.fillRect(0, 0, 480, 480, TFT_MAGENTA);
		// _sprite_content.fillScreen(TFT_MAGENTA);
	}
}

void ui_screen::clear_buffers()
{
	if (!dont_destroy_back_sprite && _sprite_back.getBuffer())
		_sprite_back.freeVirtual();

	if (_sprite_content.getBuffer())
		_sprite_content.freeVirtual();

	if (_sprite_drag.getBuffer())
		_sprite_drag.freeVirtual();

	if (_sprite_mixed.getBuffer())
		_sprite_mixed.freeVirtual();

	is_dragging = false;
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

ui_screen *ui_screen::get_navigation(Directions from)
{
	return navigation[(int)from];
}

void ui_screen::adjust_navigation_range(DRAGABLE axis, int16_t *clamp_delta_low, int16_t *clamp_delta_high)
{
	if (axis == DRAGABLE::DRAG_HORIZONTAL)
	{
		*clamp_delta_low = (navigation[(int)Directions::LEFT] == nullptr ? -20 : -480);
		*clamp_delta_high = (navigation[(int)Directions::RIGHT] == nullptr ? 20 : 480);
	}
	else if (axis == DRAGABLE::DRAG_VERTICAL)
	{
		*clamp_delta_low = (navigation[(int)Directions::UP] == nullptr ? -20 : -480);
		*clamp_delta_high = (navigation[(int)Directions::DOWN] == nullptr ? 20 : 480);
	}
}

void ui_screen::show_user_background_jpg(bool fade_in)
{
	if (!settings.config.user_wallpaper)
	{
		// User wallpapeer it turned off, so load sys wallpaper and exit
		show_random_background(fade_in);
		return;
	}

	if (squixl.user_wallpaper_buffer && squixl.user_wallpaper_length > 0)
	{
		// We have the wallpaper in memory, so use that instead of loading it and return
		show_background_jpg(squixl.user_wallpaper_buffer, squixl.user_wallpaper_length, fade_in);
		return;
	}

	// Load the user wallpaper, and store it
	settings.load_buffer_async("/user_wallpaper.jpg", [this, fade_in](bool ok, uint8_t *buffer, size_t length) {
		// Serial.printf("\n*** USER WALLAPAPER: %d - %d\n\n", ok, length);
		if (ok)
		{
			// Just in case, free previous buffer
			if (squixl.user_wallpaper_buffer)
				free(squixl.user_wallpaper_buffer);

			// Store a copy in PSRAM, so we dont have to access the async FS every time we want to load it
			squixl.user_wallpaper_buffer = (uint8_t *)ps_malloc(length);

			if (squixl.user_wallpaper_buffer)
			{
				memcpy(squixl.user_wallpaper_buffer, buffer, length);
				squixl.user_wallpaper_length = length;
				show_background_jpg(squixl.user_wallpaper_buffer, squixl.user_wallpaper_length, fade_in);
				squixl.log_heap("user jpg load");
			}
			else
			{
				// Failed to load the wallpaper, so fall back to random background and turn of user wallpaper option
				squixl.user_wallpaper_length = 0;
				settings.config.user_wallpaper = false;
				show_random_background(fade_in);
				Serial.println("Failed to allocate memory for wallpaper copy");
			}
		}
		else
		{
			Serial.println("Failed to load image");
			show_random_background(fade_in);
		}
	});
}

void ui_screen::show_background_jpg(const void *jpg, int jpg_size, bool fade_in)
{
	squixl.switching_screens = true;

	if (squixl.main_screen() != squixl.current_screen())
	{
		squixl.set_current_screen(squixl.main_screen());
		// squixl.current_screen()->refresh(true, true);
	}

	is_busy = true;

	int w, h, bpp;

	// _sprite_clean.createVirtual(480, 480, NULL, true);

	// bool has_content = false;
	// if (background_size == 0)
	// {
	// 	// _sprite_clean.fillScreen(TFT_BLACK);
	// 	_sprite_clean.fillRect(0, 0, 480, 480, TFT_BLACK);
	// }
	// else
	// {
	// 	squixl.lcd.readImage(0, 0, 480, 480, (uint16_t *)_sprite_clean.getBuffer());
	// 	delay(5);
	// 	has_content = true;
	// }

	background_size = jpg_size;

	if (squixl.jd.getJPEGInfo(&w, &h, &bpp, jpg, jpg_size))
	{
		// Serial.printf("JPG info: w %d, h %d, bpp %d\n", w, h, bpp);

		squixl.jd.loadJPEG(&_sprite_back, 0, 0, jpg, jpg_size);

		dont_destroy_back_sprite = true;

		if (fade_in)
		{
			// if (has_content)
			// {
			// 	// we only need this sprite temporarly if we are blending content
			// 	if (!_sprite_mixed.getBuffer())
			// 		_sprite_mixed.createVirtual(480, 480, NULL, true);
			// }

			for (uint8_t u8Alpha = 0; u8Alpha < 32; u8Alpha += 4)
			{
				// if (has_content)
				// {
				// 	squixl.lcd.blendSprite(&_sprite_back, &_sprite_clean, &squixl.lcd, u8Alpha);
				// 	// squixl.lcd.blendSprite(&_sprite_content, &_sprite_mixed, &squixl.lcd, 32, TFT_MAGENTA);
				// }
				// else
				// {
				squixl.lcd.blendSprite(&_sprite_back, &squixl.lcd, &squixl.lcd, u8Alpha);
				// }
			}

			// if (_sprite_mixed.getBuffer())
			// 	_sprite_mixed.freeVirtual();
		}
		else
		{
			// squixl.lcd.drawSprite(0, 0, &_sprite_back, 1.0f, -1, DRAW_TO_LCD);
			squixl.lcd.blendSprite(&_sprite_back, &squixl.lcd, &squixl.lcd, 32);
		}

		Serial.printf("Screen Wallpaper %s and displayed... loading children UI\n", (fade_in ? "faded in" : "loaded"));
	}
	else
	{
		Serial.printf("JPG not loaded - size was %d :(\n", jpg_size);
	}

	// _sprite_clean.freeVirtual();
	clear_content();

	/*
	TODO: Need to add tab group support to this?
	*/
	for (int w = 0; w < ui_children.size(); w++)
	{
		if (ui_children[w] != nullptr)
		{
			ui_children[w]->set_dirty_hard(true);
			ui_children[w]->redraw(32);
		}
	}

	if (fade_in)
	{
		for (uint8_t u8Alpha = 0; u8Alpha <= 32; u8Alpha += 2)
		{
			redraw(u8Alpha);
			// delay(2);
		}
	}
	else
	{
		redraw(32);
	}

	is_busy = false;
	next_refresh = millis();

	squixl.switching_screens = false;
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

void ui_screen::clear_content()
{
	// _sprite_content.fillScreen(TFT_MAGENTA);
	if (_sprite_content.getBuffer())
	{
		_sprite_content.fillRect(0, 0, 480, 480, TFT_MAGENTA);
	}
}

void ui_screen::clear_tabbed_children()
{
	if (ui_tab_group != nullptr)
	{
		int8_t tab_group = ui_tab_group->get_current_tab();
		for (int w = 0; w < ui_tab_group->tab_group_children[tab_group].size(); w++)
		{
			ui_control *child = static_cast<ui_control *>(ui_tab_group->tab_group_children[tab_group][w]);
			if (child != nullptr)
				child->clear_sprites();
		}
	}
}

bool ui_screen::position_children(bool force_children)
{
	bool child_dirty = false;

	int8_t tab_group = -1;
	if (ui_tab_group != nullptr)
	{
		ui_tab_group->redraw(32);
		tab_group = ui_tab_group->get_current_tab();
		current_tab_group = tab_group;

		uint8_t col = 0;
		uint8_t row = 0;

		// Serial.printf("ui_tab_group->tab_group_children[tab_group].size() %d for %d\n", ui_tab_group->tab_group_children[tab_group].size(), tab_group);

		for (int w = 0; w < ui_tab_group->tab_group_children[tab_group].size(); w++)
		{
			ui_element *child = ui_tab_group->tab_group_children[tab_group][w];
			if (child != nullptr)
			{
				child->reposition(&col, &row);
				// Serial.printf("Child is ok! Should refresh? %d\n", child->should_refresh());
				if (force_children || child->should_refresh())
				{
					if (force_children)
						child->set_dirty(true);

					if (child->redraw(32))
						child_dirty = true;
				}
			}
		}
		// Now do screen UI children
		// Serial.printf("ui_children.size()? %d\n", ui_children.size());

		for (int w = 0; w < ui_children.size(); w++)
		{
			ui_element *child = ui_children[w];
			if (child != nullptr && child->check_tab_group(-1))
			{
				// Serial.printf("Found non grid child: %s\n", child->get_title());
				if (force_children || child->should_refresh())
				{
					if (force_children)
						child->set_dirty(true);

					if (child->redraw(32))
						child_dirty = true;
				}
			}
		}
	}
	else
	{
		// Process all of this elements children without tab groups
		for (int w = 0; w < ui_children.size(); w++)
		{
			ui_element *child = ui_children[w];
			if (child != nullptr)
			{
				child->restore_reposition();
				if (force_children || child->should_refresh())
				{
					if (force_children)
						child->set_dirty(true);

					if (child->redraw(32))
						child_dirty = true;
				}
			}
		}
	}

	return child_dirty;
}

void ui_screen::refresh(bool forced, bool force_children)
{
	if (is_dragging)
	{
		// position_children(false);
		return;
	}

	unsigned long start_time = millis();

	if (is_busy)
	{
		Serial.println("BUSY!!!!");
		return;
	}

	if (keyboard.showing)
	{
		next_refresh = millis();
		return;
	}

	bool child_dirty = position_children(force_children);

	if (child_dirty || animation_manager.active_animations() > 0 || forced)
	{
		if (refresh_interval > 0)
		{
			if (animation_manager.active_animations() > 0)
				refresh_interval = 1;
			else
				refresh_interval = 20;
		}
		redraw(32);
	}

	next_refresh = millis();
	// Serial.printf("refresh time: %u ms, overlay_alpha: %u\n", (millis() - start_time), overlay_alpha);
	// Serial.printf("Nex screen refresh is at %u\n", next_refresh + refresh_interval);
}

bool ui_screen::process_touch(touch_event_t touch_event)
{
	// Serial.printf(">> SCREEN TOUCH TYPE: %d\n", touch_event.type);
	if (touch_event.type == SCREEN_DRAG_H || touch_event.type == SCREEN_DRAG_V)
	{
		if (!is_dragging)
		{
			// Serial.println("Setup for new drag");

			// drag_step_timer = millis();

			// Serial.printf("starting drag @ %u\n", drag_step_timer);

			is_dragging = true;
			drag_axis = (touch_event.type == SCREEN_DRAG_H ? DRAGABLE::DRAG_HORIZONTAL : DRAGABLE::DRAG_VERTICAL);

			if (drag_axis == DRAGABLE::DRAG_HORIZONTAL)
			{
				drag_neighbours[0] = get_navigation(Directions::LEFT);
				drag_neighbours[1] = get_navigation(Directions::RIGHT);
			}
			else
			{
				drag_neighbours[0] = get_navigation(Directions::UP);
				drag_neighbours[1] = get_navigation(Directions::DOWN);
			}

			// If we have a neighbour we are going to drag, ensure it has it's sprites initialised so we can draw it
			if (drag_neighbours[0] != nullptr)
				drag_neighbours[0]->setup_draggable_neighbour(true);

			if (drag_neighbours[1] != nullptr)
				drag_neighbours[1]->setup_draggable_neighbour(true);

			// we only need this sprite temporarly if we are blending content
			if (!_sprite_drag.getBuffer())
			{
				if (!_sprite_drag.createVirtual(480, 480, NULL, true))
				{
					squixl.log_heap("_sprite_drag");
				}
			}
		}
		drag_x = touch_event.x;
		drag_y = touch_event.y;

		last_delta_x = touch_event.d_x;
		last_delta_y = touch_event.d_y;

		if (drag_x != cached_drag_x || drag_y != cached_drag_y)
		{
			// Serial.printf("deltas %d and %d\n", last_delta_x, last_delta_y);
			draw_draggable();
		}
		cached_drag_x = drag_x;
		cached_drag_y = drag_y;

		is_dragging = true;
		return false;
	}
	else if (is_dragging)
	{
		// Serial.printf("Finished dragging with %d and %d\n", last_delta_x, last_delta_y);
		// Serial.printf("starting finish_drag @ %u - duration from start %u\n", millis(), (millis() - drag_step_timer));

		if (abs(last_delta_x) > 25 || abs(last_delta_y) > 25)
		{
			Directions finish_dir = Directions::NONE;
			if (abs(last_delta_x) > abs(last_delta_y))
				finish_dir = last_delta_x < 0 ? Directions::LEFT : Directions::RIGHT;
			else
				finish_dir = last_delta_y < 0 ? Directions::UP : Directions::DOWN;

			// Force animation of dragging the new screen into place and then switch to it.
			finish_drag(finish_dir, drag_x, drag_y);

			drag_neighbours[0] = nullptr;
			drag_neighbours[1] = nullptr;

			// squixl.log_heap("finshed drag");
		}
		else
		{
			// force Animate the screen back to 0,0 position
			cancel_drag();
		}

		// Reset drag and neighbours
		is_dragging = false;
		drag_axis = DRAGABLE::DRAG_NONE;
		return false;
	}

	int8_t tab_group = -1;
	if (ui_tab_group != nullptr)
	{
		tab_group = ui_tab_group->get_current_tab();
		if (ui_tab_group->process_touch(touch_event))
		{
			next_update = 0;
			return true;
		}

		// for (int w = 0; w < ui_tab_group->tab_group_children[tab_group].size(); w++)
		// {
		// 	ui_element *child = ui_tab_group->tab_group_children[tab_group][w];
		// 	if (child->check_tab_group(tab_group))
		// 	{
		// 		if (child->process_touch(touch_event))
		// 		{
		// 			next_update = millis() + 1000;
		// 			return true;
		// 		}
		// 	}
		// }
	}
	else
	{
		// Did any of my children recieve this touch event?
		for (int w = 0; w < ui_children.size(); w++)
		{
			if (ui_children[w]->process_touch(touch_event))
			{
				next_update = 0;
				return true;
			}
		}
	}

	if (touch_event.type == TOUCH_LONG)
	{
		if (can_cycle_background_color)
		{
			audio.play_tone(1500, 1);
			back_color = background_colors[random(5)];
			_sprite_back.fillRect(0, 0, 480, 480, back_color);
			calc_new_tints();

			int8_t tabgroup = -1;
			if (ui_tab_group != nullptr)
			{
				ui_tab_group->redraw(32);
				tabgroup = ui_tab_group->get_current_tab();

				for (int w = 0; w < ui_tab_group->tab_group_children[tab_group].size(); w++)
				{
					ui_element *child = ui_tab_group->tab_group_children[tab_group][w];
					if (child != nullptr)
						child->redraw(32);
				}
			}

			else
			{
				for (int w = 0; w < ui_children.size(); w++)
				{
					if (ui_children[w] != nullptr)
						ui_children[w]->redraw(32);
				}
			}
			// delay(10);
			redraw(32);
			return true;
		}

		return false;
	}

	return false;
}

bool ui_screen::redraw(uint8_t fade_amount, int8_t tab_group)
{
	unsigned long start_time = millis();

	// If the dialog box is open, we insert it last onto the current screen, over the content
	// This allows the content in the background to continue to update, like the time.
	if (dialogbox.is_active())
	{
		dialogbox.draw();
	}

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
			squixl.lcd.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		}
		else
		{
			if (blend_transparency)
				squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, 32, TFT_MAGENTA);
			else
				squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, 32);
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

void ui_screen::cancel_drag()
{
	// Serial.println("Cancelling drag");

	while (drag_x != 0 || drag_y != 0)
	{
		drag_x = round(drag_x / 2);
		drag_y = round(drag_y / 2);
		// Serial.printf(">Screen drag back %d,%d\n", drag_x, drag_y);
		draw_draggable();
	}

	if (_sprite_drag.getBuffer())
		_sprite_drag.freeVirtual();

	clean_neighbour_sprites();
}

void ui_screen::finish_drag(Directions direction, int16_t dx, int16_t dy)
{
	// float from_x = (float)dx;
	int to_x = 0;
	// float from_y = (float)dy;
	int to_y = 0;
	switch (direction)
	{
	case LEFT:
		to_x = -480;
		break;
	case RIGHT:
		to_x = 480;
		break;
	case UP:
		to_y = -480;
		break;
	case DOWN:
		to_y = 480;
		break;
	}

	squixl.switching_screens = true;

	while (drag_x != to_x || drag_y != to_y)
	{
		drag_x = round(drag_x + (to_x - drag_x) / 2.0);
		drag_y = round(drag_y + (to_y - drag_y) / 2.0);
		draw_draggable();
	}

	is_dragging = false;

	squixl.set_current_screen(navigation[(int)direction]);
	squixl.current_screen()->clean_neighbour_sprites();

	squixl.switching_screens = false;
	squixl.current_screen()->refresh(true, true);
}

void ui_screen::clean_neighbour_sprites()
{
	for (int i = 0; i < 4; i++)
	{
		if (navigation[i] != nullptr)
		{
			// Serial.printf("clearing buffers in neighbour %d\n", i);
			navigation[i]->clear_buffers();
		}
	}
}

void ui_screen::draw_draggable()
{
	if (_sprite_drag.getBuffer())
	{
		squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &_sprite_content, 32, TFT_MAGENTA);
		_sprite_drag.drawSprite(drag_x, drag_y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);

		if (drag_x != 0)
		{
			if (drag_x > 0)
			{
				if (drag_neighbours[1] == nullptr)
					_sprite_drag.fillRect(0, 0, drag_x, 480, 0, DRAW_TO_RAM);
				else
					drag_neighbours[1]->draw_draggable_neighbour(&_sprite_drag, drag_x - 480, 0);
			}
			else
			{
				if (drag_neighbours[0] == nullptr)
					_sprite_drag.fillRect(480 - abs(drag_x), 0, abs(drag_x), 480, 0, DRAW_TO_RAM);
				else
					drag_neighbours[0]->draw_draggable_neighbour(&_sprite_drag, drag_x + 480, 0);
			}
		}
		else
		{
			if (drag_y > 0)
			{
				if (drag_neighbours[1] == nullptr)
					_sprite_drag.fillRect(0, 0, 480, drag_y, 0, DRAW_TO_RAM);
				else
					drag_neighbours[1]->draw_draggable_neighbour(&_sprite_drag, 0, drag_y - 480);
			}
			else
			{
				if (drag_neighbours[0] == nullptr)
					_sprite_drag.fillRect(0, 480 - abs(drag_y), 480, abs(drag_y), 0, DRAW_TO_RAM);
				else
					drag_neighbours[0]->draw_draggable_neighbour(&_sprite_drag, 0, drag_y + 480);
			}
		}

		squixl.lcd.blendSprite(&_sprite_drag, &squixl.lcd, &squixl.lcd, 32);
		// squixl.lcd.drawSprite(0, 0, &_sprite_drag, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		// Serial.printf("missing sprite? %d, drag? %d, mixed? %d\n", is_dragging, _sprite_drag.getBuffer(), _sprite_mixed.getBuffer());
	}

	// Serial.printf("refresh time (draw borders): %u ms\n", (millis() - start_time));
}

void ui_screen::draw_draggable_neighbour(BB_SPI_LCD *sprite, int16_t dx, int16_t dy)
{
	if (_sprite_content.getBuffer() && _sprite_back.getBuffer())
	{
		sprite->blendSprite(&_sprite_content, &_sprite_back, &_sprite_content, 32, TFT_MAGENTA);
		sprite->drawSprite(dx, dy, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);
	}
}

void ui_screen::setup_draggable_neighbour(bool state)
{
	if (state)
	{
		create_buffers();
		if (position_children(true))
		{
			// Serial.println("Children ready for drag");
		}
	}
	else if (!state)
	{
		// not being called anymore
		clear_buffers();
	}
}
