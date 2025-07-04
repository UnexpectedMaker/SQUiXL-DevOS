#include "ui/controls/ui_control_slider.h"
#include "ui/ui_screen.h"

void ui_control_slider::update_values()
{

	if (value_type == VALUE_TYPE::INT)
	{
		auto *opt = static_cast<SettingsOptionIntRange *>(setting_option);
		opt->log_data();
		current_value = (float)opt->get();
		value_min = (float)opt->get_min_value();
		value_max = (float)opt->get_max_value();
		value_step = (float)opt->get_step_value();
		// Serial.printf("\n@@@ current_value (INT): %0.1f - min: %0.1f, max %0.1f \n\n", current_value, value_min, value_max);
	}
	else if (value_type == VALUE_TYPE::FLOAT)
	{
		auto *opt = static_cast<SettingsOptionFloatRange *>(setting_option);
		current_value = opt->get();
		value_min = opt->get_min_value();
		value_max = opt->get_max_value();
		value_step = opt->get_step_value();
		// Serial.printf("\n@@@ current_value (FLOAT): %0.1f - min: %0.1f, max %0.1f \n\n", current_value, value_min, value_max);
	}
}

void ui_control_slider::set_options_data(SettingsOptionBase *sett)
{
	if (sett == nullptr)
		return;

	setting_option = sett;

	update_values();

	_title = setting_option->fieldname.c_str();
	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 0, &char_width_title, &char_height_title);
	title_len_pixels = _title.length() * char_width_title;

	set_label_sizes();
	set_draggable(DRAGGABLE::DRAG_HORIZONTAL);
}

void ui_control_slider::set_label_sizes()
{
	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &char_width, &char_height);

	if (value_type == VALUE_TYPE::INT)
	{
		value_min_text = string_from_int((int)value_min, prefix.c_str(), suffix.c_str());
		value_max_text = string_from_int((int)value_max, prefix.c_str(), suffix.c_str());
	}
	else
	{
		value_min_text = string_from_float(value_min, prefix.c_str(), suffix.c_str());
		value_max_text = string_from_float(value_max, prefix.c_str(), suffix.c_str());
	}

	value_min_len_pixels = value_min_text.length() * char_width;
	value_max_len_pixels = value_max_text.length() * char_width;
}

bool ui_control_slider::redraw(uint8_t fade_amount, int8_t tab_group)
{
	// This is busy if something else is drawing this
	if (is_busy)
	{
		// Serial.println("Can't refresh, busy...");
		return false;
	}

	is_busy = true;

	if (!_sprite_content.getBuffer())
	{
		_sprite_content.createVirtual(_w, _h, NULL, true);
	}

	// Clear the content sprite
	_sprite_content.fillRect(0, 0, _w, _h, TFT_MAGENTA);

	update_values();

	// This is only needed once, and noly if the control is not bound to an OptionSetting
	if (char_width == 0)
	{
		set_label_sizes();
	}

	// Calculate the string pixel sizes to allow for text centering
	if (value_len_pixels == 0 || value_changed)
	{
		if (value_type == VALUE_TYPE::INT)
		{
			value_text = string_from_int(current_value, prefix.c_str(), suffix.c_str());
		}
		else
		{
			value_text = string_from_float(current_value, prefix.c_str(), suffix.c_str());
		}

		uint8_t string_len = value_text.length();
		value_len_pixels = string_len * char_width;
		value_changed = false;
		// Serial.printf("\n>> string %s, len %d, pixels %d\n", value_text.c_str(), string_len, value_len_pixels);
	}

	if (value_percentage < 0.0f)
	{
		value_percentage = normalise_int(current_value, value_min, value_max);
	}

	// Control background
	_sprite_content.fillRoundRect(0, 0, _w, _h, 9, static_cast<ui_screen *>(get_ui_parent())->dark_tint[1], DRAW_TO_RAM);

	// If the control has a title, show it at the bottom center
	if (_title.length() > 0)
	{
		_sprite_content.setFreeFont(UbuntuMono_R[0]);
		_sprite_content.setCursor((_w / 2) - (title_len_pixels / 2), _h - 6);
		_sprite_content.setTextColor(static_cast<ui_screen *>(get_ui_parent())->light_tint[5], -1);
		_sprite_content.print(_title.c_str());
		// _sprite_content.setCursor((_w / 2) + 8, _h - 6);
	}

	// Control main value
	_sprite_content.setFreeFont(UbuntuMono_R[2]);
	_sprite_content.setTextColor(TFT_WHITE, -1);
	_sprite_content.setCursor((_w / 2) - (value_len_pixels / 2), char_height + 5);
	_sprite_content.print(value_text.c_str());

	uint16_t x_pos_center = 20 + ((_w - 40) * value_percentage);

	if (suffix == "%")
	{
		_sprite_content.drawRect(10, _h / 2 + 5, _w - 20, 2, static_cast<ui_screen *>(get_ui_parent())->dark_tint[3]);
		_sprite_content.drawRect(10, _h / 2 + 5, (x_pos_center - 1), 2, TFT_WHITE);
	}
	else
		_sprite_content.drawRect(10, _h / 2 + 5, _w - 20, 2, TFT_WHITE);

	_sprite_content.setTextColor(static_cast<ui_screen *>(get_ui_parent())->light_tint[3], -1);
	_sprite_content.setCursor(10, char_height + 5);
	_sprite_content.print(value_min_text.c_str());
	_sprite_content.setCursor(_w - 10 - value_max_len_pixels, char_height + 5);
	_sprite_content.print(value_max_text.c_str());
	_sprite_content.fillCircle(x_pos_center, (_h / 2) + 5, 8, TFT_WHITE, DRAW_TO_RAM);

	get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1, DRAW_TO_RAM);

	next_refresh = millis();

	is_dirty = false;
	is_busy = false;

	// this is not a self updating element, so we never need to let the parent know its been update
	return false;
}

void ui_control_slider::set_value_text(const char *_text)
{
	value_text = _text;
}

void ui_control_slider::set_min_max(float _min, float _max, float _step)
{
	value_min = _min;
	value_max = _max;
	value_step = _step;
	set_draggable(DRAGGABLE::DRAG_HORIZONTAL);
}

bool ui_control_slider::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_DRAG)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			// 1. compute continuous value 0…100
			value_percentage = normalise_int(touch_event.x, _x + 20, _x + _w - 40);
			float raw = (value_max - value_min) * value_percentage + value_min;

			// 2. snap it to nearest step
			//    e.g. steps = 10, halfStep = 5
			int steps = int((raw - value_min + value_step / 2.0f) / value_step);
			float snapped = value_min + steps * value_step;

			// 3. clamp just in case
			current_value = constrain(snapped, value_min, value_max);

			if (value_type == VALUE_TYPE::INT)
				static_cast<SettingsOptionInt *>(setting_option)->update((int)current_value);
			else if (value_type == VALUE_TYPE::FLOAT)
				static_cast<SettingsOptionFloat *>(setting_option)->update(current_value);

			value_changed = true;

			redraw(32);
			static_cast<ui_screen *>(get_ui_parent())->refresh(true);

			return false;
		}
		else
		{
			// Serial.printf("\n\n %d,%d - %d, %d, %d, %d\n\n", touch_event.x, touch_event.y, _x, _y, _w, _h);
		}
	}
	else if (touch_event.type == TOUCH_DRAG_END)

	{

		// if (callbackFunction != nullptr)
		// 	callbackFunction();

		return true;
	}

	return false;
}
