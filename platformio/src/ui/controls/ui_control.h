#pragma once

#include "ui/ui_element.h"

enum VALUE_TYPE
{
	INT,
	BOOL,
	FLOAT,
	STRING,
};

class ui_control : public ui_element
{
	public:
		using CallbackFunction = void (*)();

		// void create(uint16_t _pos_x, uint16_t _pos_y, uint16_t _width, uint16_t _height);
		void create(uint16_t _pos_x, uint16_t _pos_y, uint16_t _width = 480, uint16_t _height = 480, const char *title = "");
		// void create_on_grid(uint8_t _col, uint8_t _row, uint8_t _span_c, uint8_t _span_r, const char *title = "");
		void create_on_grid(uint8_t _span_c, uint8_t _span_r, const char *title = "");
		void set_callback(CallbackFunction callback);
		void set_control_icon(const void *image_data, int image_data_size);
		void set_grid_padding(uint8_t padding) { grid_padding = padding; }
		void clear_sprites();

		// Virtual funcs
		virtual void set_options_data(SettingsOptionBase *sett) { setting_option = sett; }
		virtual void set_label_sizes();

	protected:
		std::string string_from_float(float val, const char *prefix, const char *suffix);
		std::string string_from_int(int val, const char *prefix, const char *suffix);

		// std::string _title = "";
		CallbackFunction callbackFunction;
		BB_SPI_LCD _control_icon;

		SettingsOptionBase *setting_option = nullptr;

		uint8_t char_width = 0;
		uint8_t char_height = 0;

		uint8_t char_width_title = 0;
		uint8_t char_height_title = 0;
		uint16_t title_len_pixels = 0;

		uint8_t grid_padding = 10;
		uint8_t col_width = 80;
		uint8_t row_height = 80;

		// uint16_t y_tab_group_offset = 40;

	private:
		//
};
