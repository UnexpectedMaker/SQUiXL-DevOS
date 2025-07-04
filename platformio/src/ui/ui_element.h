#pragma once

#include "squixl.h"
#include <map>

class ui_control_tabgroup;

enum TEXT_ALIGN
{
	ALIGN_LEFT = 0,
	ALIGN_CENTER = 1,
	ALIGN_RIGHT = 2,
};

struct UI_PADDING
{
		int8_t left = 10;
		int8_t right = 10;
		int8_t top = 10;
		int8_t bottom = 10;
};

class ui_element
{
		using CallbackFunction = void (*)();

	public:
		// Drawing stuff
		virtual bool redraw(uint8_t fade_amount, int8_t tab_group = -1) { return false; }

		virtual void capture_clean_sprite();
		virtual void set_dirty(bool state) { is_dirty = state; }
		virtual void set_dirty_hard(bool state) { is_dirty_hard = state; }

		virtual void about_to_show_screen() {};
		virtual void about_to_close_screen() {};

		// Touch related stuff
		virtual bool process_touch(touch_event_t touch_event) { return false; }

		ui_element *find_touched_element(uint16_t x, uint16_t y, int8_t tabgroup = -1);
		bool check_bounds(uint16_t x, uint16_t y);

		void set_callback(CallbackFunction callback);
		void set_refresh_interval(uint16_t interval = 0);

		bool should_refresh();

		void add_child_ui(ui_element *child, int8_t tab_group = -1);
		ui_element *get_ui_parent();

		const char *get_title();

		void fade(float from, float to, unsigned long duration, bool set_dirty, bool invalidate_cache, std::function<void()> completion_callback = nullptr);

		uint16_t calc_text_size(const char *text, const GFXfont *font, int *_text_w, int *_text_h);

		int16_t pos_x() { return _x; }
		int16_t pos_y() { return _y; }

		uint16_t refresh_interval = 0;
		unsigned long next_refresh = 0;

		bool is_setup = false;

		float normalise_float(float current, float min, float max);
		float normalise_int(int x, int minX, int maxX);

		DRAGGABLE is_dragable() { return drag_able; }
		void set_draggable(DRAGGABLE state) { drag_able = state; }
		bool is_element_dragging() { return is_dragging; }

		void set_touchable(bool state) { is_touchable = state; }
		void set_delayed_frst_draw(int val) { delay_first_draw = millis() + val; }
		bool touchable() { return is_touchable; }

		int8_t get_tab_group() { return element_tab_group; }
		void set_tab_group(int8_t group) { element_tab_group = group; }
		bool check_tab_group(int8_t group) { return element_tab_group == group || group < 0; }

		void reposition(uint8_t *_col, uint8_t *_row);
		void restore_reposition();

		int drag_dir = -1;

		UI_PADDING padding;

		uint8_t touch_padding = 0;

		BB_SPI_LCD _sprite_content;
		BB_SPI_LCD _sprite_clean;
		BB_SPI_LCD _sprite_back;
		BB_SPI_LCD _sprite_mixed;

		// My direct UI parent that I am a child of
		ui_element *ui_parent = nullptr;
		// Any UI children that I am the parent of
		std::vector<ui_element *> ui_children;
		std::map<int8_t, std::vector<ui_element *>> tab_group_children;

	protected:
		int16_t _x = 0; // X position
		int16_t _y = 0; // Y position
		int16_t _w = 0; // Width
		int16_t _h = 0; // Height
		int16_t _c = 0; // Color (565)
		uint8_t _t = 0; // Transparency
		uint8_t _b = 0; // Blur Count

		int16_t _origional_x = 0; // X position
		int16_t _origional_y = 0; // Y position

		bool repositioned = false;

		std::string _title = "";

		TEXT_ALIGN _align; // Heading alignment

		CallbackFunction callback_func;

		unsigned long next_update = 0;
		unsigned long next_click_update = 0;
		unsigned long delay_first_draw = 0;

		bool is_touchable = true;
		DRAGGABLE drag_able = DRAGGABLE::DRAG_NONE;

		unsigned long click_hold_start_timer = 0;

		bool is_dirty_hard = true; // Do I need to capture my clean backgrouns sprite
		bool is_dirty = true;	   // Do I need to re-draw my content?
		bool is_busy = false;	   // Am I currently re-drawing?
		bool is_aniamted_cached = false;
		bool has_never_drawn = true;
		bool is_dragging = false;

		int8_t element_tab_group = -1;
};
