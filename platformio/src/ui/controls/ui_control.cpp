#include "ui/controls/ui_control.h"
#include <cstdio>

void ui_control::create(uint16_t _pos_x, uint16_t _pos_y, uint16_t _width, uint16_t _height, const char *title)
{
	_x = _pos_x;
	_y = _pos_y;
	_w = _width;
	_h = _height;

	touch_padding = 10;

	_title = title;

	// _sprite_content.createVirtual(_w, _h, NULL, true);
	// _sprite_clean.createVirtual(_w, _h, NULL, true);
	// _sprite_mixed.createVirtual(_w, _h, NULL, true);

	// Serial.printf("created control %d, %d, %d, %d\n", _x, _y, _w, _h);

	if (_title.length() > 0)
		set_label_sizes();
}

void ui_control::create_on_grid(uint8_t _col, uint8_t _row, uint8_t _span_c, uint8_t _span_r, const char *title)
{

	/*
			uint8_t grid_padding = 10;
		uint8_t col_width = 160;
		uint8_t row_height = 80;
		*/

	uint16_t _pos_x = (_col * col_width + grid_padding);
	uint16_t _pos_y = (_row * row_height + grid_padding);

	// Need to ensure the col span is not wider than the screen based on the selected col
	if (_col + _span_c > 3)
		_span_c = 3 - _col;

	if (_row + _span_r > 6)
		_span_r = 6 - _row;

	uint16_t _width = (col_width * _span_c - grid_padding - grid_padding);
	uint16_t _height = (row_height * _span_r - grid_padding - grid_padding);

	create(_pos_x, _pos_y, _width, _height, title);
}

void ui_control::create_on_grid(uint8_t _span_c, uint8_t _span_r, const char *title)
{

	/*
		uint8_t grid_padding = 10;
		uint8_t col_width = 80; // new col width based on 6 colums so can do 2 wide for 3 cols, or 3 wide for 2 cols etc
		uint8_t row_height = 80;
		*/
	if (_span_c < 2)
		_span_c = 2;

	uint16_t _width = (col_width * _span_c - grid_padding * 2);
	uint16_t _height = (row_height * _span_r - grid_padding * 2);

	// Serial.printf("%s is grid created as _w %d, _h %d, _span %d\n", get_title(), _width, _height, _span_c);

	create(0, 0, _width, _height, title);
}

void ui_control::clear_sprites()
{
	if (_sprite_content.getBuffer())
		_sprite_content.freeVirtual();
}

void ui_control::set_label_sizes()
{
	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 0, &char_width_title, &char_height_title);
	title_len_pixels = _title.length() * char_width_title;
}

void ui_control::set_control_icon(const void *image_data, int image_data_size)
{
	_control_icon.createVirtual(48, 48, NULL, true);
	squixl.loadPNG_into(&_control_icon, 0, 0, image_data, image_data_size);
}

void ui_control::set_callback(CallbackFunction callback) { callbackFunction = callback; }

std::string ui_control::string_from_float(float val, const char *prefix, const char *suffix)
{
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%s%.1f%s", prefix, val, suffix);
	return std::string(buf);
}

std::string ui_control::string_from_int(int val, const char *prefix, const char *suffix)
{
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%s%d%s", prefix, val, suffix);
	return std::string(buf);
}
