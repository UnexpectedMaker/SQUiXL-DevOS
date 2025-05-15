#pragma once

#include "ui/ui_element.h"
#include <map>
#include <vector>
#include "WiFi.h"

class ui_screen;
extern std::vector<ui_screen *> screens;

class ui_screen : public ui_element
{
	public:
		// ui_screen();

		void setup(uint16_t back_color, bool add = true);

		void set_navigation(Directions from, ui_screen *screen, bool set_reversed = false);

		using CallbackFunction = void (*)();

		// void show_background_png(const void *png, int png_size, bool fade = true);
		void show_background_jpg(const void *jpg, int jpg_size, bool fade = true);
		void show_next_background();
		void show_random_background(bool fade = true);

		void refresh(bool forced = false, bool force_children = false);

		// Vitrual Funcs
		bool redraw(uint8_t fade_amount) override;

		void calc_new_tints();
		void set_can_cycle_back_color(bool state) { can_cycle_background_color = state; }

		bool process_touch(touch_event_t touch_event) override;

		uint16_t background_color() { return back_color; }

		void show_overlay(bool show, unsigned long duration, std::function<void()> completion_callback);
		void fade_overlay(uint8_t fade_amount);

		void animate_pos(Directions direction, unsigned long duration, tween_ease_t ease, std::function<void()> completion_callback);

		uint16_t dark_tint[8];
		uint16_t light_tint[8];

	protected:
		int background_size = 0;
		uint16_t back_color = 0;
		bool can_cycle_background_color = false;

		bool blend_transparency = true;
		uint8_t overlay_alpha = 0;
		bool rebuild_mixed_sprite = false;
		bool switching_screens = false;

		ui_screen *navigation[4] = {nullptr, nullptr, nullptr, nullptr};
};
