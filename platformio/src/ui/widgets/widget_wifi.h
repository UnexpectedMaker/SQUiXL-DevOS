// #pragma once

// #include "ui/ui_element.h"

// class widgetWiFi: public ui_element
// {
// 	public:
// 		void create(int16_t x, int16_t y, uint16_t color, TEXT_ALIGN alignment);
// 		// void update(const char *title);
// 		// void move(int16_t x, int16_t y);

// 		// void show(bool fade = false);

// 		// Virtual Funcs
// 		bool redraw(uint8_t fade_amount) override;
// 		void slow_fade();

// 	protected:
// 		int16_t _x;	 // draw pos x
// 		int16_t _y;	 // draw pos y
// 		uint16_t _w; // calculated width
// 		uint16_t _h; // calculated height
// 		uint16_t _c; // color

// 		TEXT_ALIGN _align; // draw alignment

// 		int16_t _adj_x; // alignment adjusted draw pos x
// 		int16_t _adj_y; // alignment adjusted draw pos y

// 		uint8_t current_fade = 0;
// 		bool fade_dir = true;

// 		BB_SPI_LCD _sprite_back, _sprite_content, _sprite_mixed;

// 		bool calculate_text_size(bool forced = false);

// 		unsigned long next_update = 0;

// 		std::string _time_string = "--/--/--";
// 		std::string _date_string = "--/--/--";

// 		uint16_t timeh;
// 		uint16_t dateh;
// 		uint16_t timew;
// 		uint16_t datew;

// 		bool has_data = false;
// 		bool should_redraw = false;
// 		bool is_setup = false;
// };

// extern widgetWiFi widget_wifi;
