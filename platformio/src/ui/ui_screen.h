#pragma once

#include "ui/ui_element.h"
#include "ui/controls/ui_control_tabgroup.h"
#include <map>
#include <vector>
#include "WiFi.h"

extern std::vector<ui_screen *> screens;

class ui_screen : public ui_element
{
	public:
		void setup(uint16_t back_color, bool add = true);

		void set_navigation(Directions from, ui_screen *screen, bool set_reversed = false);
		ui_screen *get_navigation(Directions from);
		void adjust_navigation_range(DRAGABLE axis, int16_t *clamp_delta_low, int16_t *clamp_delta_high);

		using CallbackFunction = void (*)();

		// void show_background_png(const void *png, int png_size, bool fade = true);
		void show_background_jpg(const void *jpg, int jpg_size, bool fade = true);
		void show_user_background_jpg(bool fade = true);
		void show_next_background();
		void show_random_background(bool fade = true);

		void refresh(bool forced = false, bool force_children = false);
		void clear_content();
		void clear_tabbed_children();

		void create_buffers();
		void clear_buffers();

		void set_page_tabgroup(ui_control_tabgroup *child);

		// Vitrual Funcs
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;

		void calc_new_tints();
		void set_can_cycle_back_color(bool state) { can_cycle_background_color = state; }

		bool process_touch(touch_event_t touch_event) override;

		uint16_t background_color() { return back_color; }

		void show_overlay(bool show, unsigned long duration, std::function<void()> completion_callback);
		void fade_overlay(uint8_t fade_amount);
		void take_over(bool state) { taken_over = state; }
		bool is_taken_over() { return taken_over; }

		// void animate_pos(Directions direction, unsigned long duration, tween_ease_t ease, std::function<void()> completion_callback);

		void finish_drag(Directions direction, int16_t dx, int16_t dy);
		void cancel_drag();

		int8_t get_tab_group_index();
		bool position_children(bool force_children = false);

		uint16_t dark_tint[8];
		uint16_t light_tint[8];

		ui_control_tabgroup *ui_tab_group = nullptr;

	protected:
		int background_size = 0;
		uint16_t back_color = 0;
		bool can_cycle_background_color = false;

		bool blend_transparency = true;
		uint8_t overlay_alpha = 0;
		bool switching_screens = false;
		bool taken_over = false;

		int16_t drag_x = 0;
		int16_t drag_y = 0;
		int16_t cached_drag_x = 0;
		int16_t cached_drag_y = 0;

		int16_t last_delta_x = 0;
		int16_t last_delta_y = 0;

		unsigned long drag_step_timer = 0;

		bool is_dragging = false;
		DRAGABLE drag_axis = DRAG_NONE;
		ui_screen *drag_neighbours[2] = {nullptr, nullptr};

		void draw_draggable();
		void setup_draggable_neighbour(bool state);
		void draw_draggable_neighbour(BB_SPI_LCD *sprite, int16_t dx, int16_t dy);

		void clean_neighbour_sprites();

		BB_SPI_LCD _sprite_drag;

		int8_t current_tab_group = -1;

		bool dont_destroy_back_sprite = false;

		ui_screen *navigation[4] = {nullptr, nullptr, nullptr, nullptr};
};
