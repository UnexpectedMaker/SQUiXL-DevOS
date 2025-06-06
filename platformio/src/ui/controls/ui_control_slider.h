#pragma once

#include "ui/controls/ui_control.h"

class ui_control_slider : public ui_control
{
	public:
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event) override;
		void set_value_text(const char *_text);
		void set_min_max(float _min, float _max, float _step);
		void set_prefix(const char *pre) { prefix = pre; }
		void set_suffix(const char *suf) { suffix = suf; }
		void set_value_type(VALUE_TYPE type) { value_type = type; }

		// Virtual funcs
		void set_options_data(SettingsOptionBase *sett) override;
		void set_label_sizes() override;
        void update_values();

	protected:
		std::string value_text = "50";
		std::string value_min_text = "0";
		std::string value_max_text = "0";
		std::string prefix = "";
		std::string suffix = "";
		VALUE_TYPE value_type = VALUE_TYPE::INT;
		float current_value = 50;
		float value_min = 0;
		float value_max = 100;
		float value_step = 1.0;
		float value_percentage = -1.0f;
		uint16_t value_len_pixels = 0;
		uint16_t value_min_len_pixels = 0;
		uint16_t value_max_len_pixels = 0;
		bool value_changed = true;
		bool flash_left = false;
		bool flash_right = false;
};
