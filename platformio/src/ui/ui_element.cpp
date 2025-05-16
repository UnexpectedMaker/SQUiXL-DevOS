#include "ui/ui_element.h"
#include "settings/settings.h"

void ui_element::add_child_ui(ui_element *child, int8_t tab_group)
{
	ui_children.push_back(child);
	child->ui_parent = this;
	child->set_tab_group(tab_group);
	tab_group_children[tab_group].push_back(child);
}

void ui_element::set_refresh_interval(uint16_t interval)
{
	refresh_interval = interval;
	next_refresh = millis();
}

bool ui_element::should_refresh()
{
	if (has_never_drawn)
	{
		has_never_drawn = false;
		return true;
	}

	if (refresh_interval == 0)
	{
		// Serial.println("Interval not set on");
		return false;
	}

	if (is_busy)
		return false;

	bool ok = false;
	if (millis() - next_refresh > refresh_interval)
	{
		// Serial.printf("Interval %d\n", refresh_interval);
		next_refresh = millis();
		ok = true;
	}

	return ok;
}

void ui_element::reposition(uint8_t *_col, uint8_t *_row)
{
	if (repositioned)
		return;

	repositioned = true;
	// Back up original values
	_origional_x = _x;
	_origional_y = _y;

	uint8_t grid_padding = 10;
	uint8_t col_width = 160;
	uint8_t row_height = 80;

	uint8_t _span_c = _w / (col_width - grid_padding - grid_padding);
	uint8_t _span_r = _h / (row_height - grid_padding - grid_padding);

	// Serial.printf("element _w %d, col_width %d, span_c %d, new col %d, rew row %d\n", _w, (col_width - grid_padding - grid_padding), _span_c, (*_col + _span_c), *_row);

	// Need to ensure the col span is not wider than the screen based on the selected col
	if (*_col + _span_c > 3)
	{
		*_col = 0;
		*_row = *_row + 1;
		if (*_row > 5)
		{
			Serial.printf("element Error 1 _col %d, _row %d\n", *_col, *_row);

			*_row = 99;
			*_col = 99;
			return;
		}
	}

	_x = (*_col * col_width + grid_padding);
	_y = (*_row * row_height + grid_padding) + 60; // tab group offset

	// Serial.printf("element repositioned to col %d, row %d, span_c %d\n", *_col, *_row, _span_c);

	*_col += _span_c;
	if (*_col > 2)
	{
		*_col = 0;
		*_row = *_row + 1;
		if (*_row > 5)
		{
			Serial.printf("element Error 2 _col %d, _row %d\n", *_col, *_row);
			*_row = 99;
			*_col = 99;
		}
	}
}

void ui_element::restore_reposition()
{
	if (repositioned)
	{
		repositioned = false;
		// Restore original values
		_x = _origional_x;
		_y = _origional_y;
	}
}

void ui_element::capture_clean_sprite()
{
	squixl.lcd.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
	is_dirty_hard = false;
}

uint16_t ui_element::calc_text_size(const char *text, const GFXfont *font, int *_text_w, int *_text_h)
{
	int16_t tempx;
	int16_t tempy;
	uint16_t tempw;
	uint16_t temph;

	BB_SPI_LCD text_test;
	text_test.createVirtual(100, 100, NULL, true);
	text_test.setFreeFont(font);
	text_test.getTextBounds(text, 0, 0, &tempx, &tempy, &tempw, &temph);

	*_text_w = (int)tempw;
	*_text_h = (int)temph;

	// Serial.printf("calc_text_size: %s - New Width: %u, Height: %u\n", String(text), *_text_w, *_text_h);

	text_test.freeVirtual();

	return tempw;
}

const char *ui_element::get_title()
{
	return (_title.c_str());
}

bool ui_element::check_bounds(uint16_t touch_x, uint16_t touch_y)
{
	// Serial.printf("%d >= %d - %d (%d) && %d < %d + %d + %d (%d)\n", touch_y, _y, touch_padding, (_y - touch_padding), touch_y, _y, _h, touch_padding, (_y + _h + touch_padding));

	// Serial.printf("%d >= %d && %d < %d ? %d\n", touch_y, (_y - touch_padding), touch_y, (_y + _h + touch_padding), (touch_y >= _y - touch_padding && touch_y < _y + _h + touch_padding));

	return (touch_x >= _x - touch_padding && touch_x < _x + _w + touch_padding && touch_y >= _y - touch_padding && touch_y < _y + _h + touch_padding);
}

ui_element *ui_element::find_touched_element(uint16_t x, uint16_t y, int8_t tabgroup)
{
	// Serial.printf("Number of UI element children %d\n", ui_children.size());
	for (int i = 0; i < ui_children.size(); i++)
	{
		if (ui_children[i] != nullptr && ui_children[i]->touchable() && ui_children[i]->check_tab_group(tabgroup))
		{
			// Serial.printf("Checking UI element %s\n", ui_children[i]->get_title());
			if (ui_children[i]->check_bounds(x, y))
				return ui_children[i];
		}
	}

	// Serial.println("Nope, returning this");

	// if (!squixl.showing_settings)
	if (ui_parent != nullptr)
		return ui_parent;

	return this;

	return nullptr;
}

float ui_element::normalise_float(float current, float min, float max)
{
	if (max == min)
		return 0.0f;					  // avoid divide‑by‑zero
	return (current - min) / (max - min); // yields 0.0…1.0
}

float ui_element::normalise_int(int x, int minX, int maxX)
{
	if (maxX == minX)
		return 0.0f;
	if (x < minX)
		x = minX;
	else if (x > maxX)
		x = maxX;

	return (x - minX) / float(maxX - minX); // cast to float
}

// Animation

void ui_element::fade(float from, float to, unsigned long duration, bool set_dirty, bool invalidate_cache, std::function<void()> completion_callback)
{
	is_dirty_hard = set_dirty;
	is_aniamted_cached = !invalidate_cache;
	// audio.play_tone(1000, 1);
	animation_manager.add_animation(new tween_animation(from, to, duration, tween_ease_t::EASE_LINEAR, [this](float opacity) {
			uint8_t fade_amount = int(32.0 * opacity);
			this->redraw(fade_amount); }, [completion_callback]() {
				if (completion_callback) {
					completion_callback();
				} }));
}
