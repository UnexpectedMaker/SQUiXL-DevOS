#include "ui/ui_element.h"
#include "settings/settings.h"

// static ui_element *current_ui_element = nullptr;

void ui_element::add_child_ui(ui_element *child)
{
	ui_children.push_back(child);
	child->ui_parent = this;
}

// void ui_element::set_parent_sprite(BB_SPI_LCD *sprite)
// {
// 	parent_sprite = sprite;
// }

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
		next_refresh = millis();
		ok = true;
	}

	return ok;
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
	return (touch_x >= _x - touch_padding && touch_x < _x + _w + touch_padding && touch_y >= _y - touch_padding && touch_y < _y + _h + touch_padding);
}

ui_element *ui_element::find_touched_element(uint16_t x, uint16_t y)
{
	// Serial.printf("Number of UI element children %d\n", ui_children.size());
	for (int i = 0; i < ui_children.size(); i++)
	{
		if (ui_children[i] != nullptr && ui_children[i]->touchable())
		{
			// Serial.printf("Checking UI element %s\n", ui_children[i]->get_title());
			if (ui_children[i]->check_bounds(x, y))
				return ui_children[i];
		}
	}

	if (!squixl.showing_settings)
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
