#include "ui/dashboard/ui_gauge.h"

void ui_gauge::create(int16_t x, int16_t y, int16_t w, int16_t h)
{
	_x = x;
	_y = y;
	_w = w;
	_h = h;

	_sprite_clean.createVirtual(_w, _h, NULL, true);
	_sprite_back.createVirtual(_w, _h, NULL, true);
	_sprite_content.createVirtual(_w, _h, NULL, true);
	_sprite_mixed.createVirtual(_w, _h, NULL, true);

	// is_dirty = true;
	is_busy = false;

	_sprite_clean.readImage(ui_parent->pos_x() + _x, ui_parent->pos_y() + y, _w, _h, (uint16_t *)_sprite_content.getBuffer());
}

void ui_gauge::set_payload_index(const char *key, uint16_t index, const char *device)
{
	_payload_key = key;
	_payload_index = index;

	// need to determine which graphics to load here, but for now, only if temp
	load_graphics(device);
}

void ui_gauge::load_graphics(const char *device)
{
	if (std::string(device) == "temperature")
	{
		// loadPNG_into(BB_SPI_LCD *sprite, int start_x, int start_y, const void *image_data, int image_data_size)
		_graphic_back.createVirtual(_w, _h, NULL, true);
		squixl.loadPNG_into(&_graphic_back, 0, 0, temp_guage_arc, sizeof(temp_guage_arc));
		_graphic_front.createVirtual(_w, _h, NULL, true);
		squixl.loadPNG_into(&_graphic_front, 0, 0, temp_guage_dial, sizeof(temp_guage_dial));
		// Serial.println("Loaded PNG Graphics");
	}
	else if (std::string(device) == "humidity")
	{
		// loadPNG_into(BB_SPI_LCD *sprite, int start_x, int start_y, const void *image_data, int image_data_size)
		_graphic_back.createVirtual(_w, _h, NULL, true);
		squixl.loadPNG_into(&_graphic_back, 0, 0, humidity_guage_arc, sizeof(humidity_guage_arc));
		_graphic_front.createVirtual(_w, _h, NULL, true);
		squixl.loadPNG_into(&_graphic_front, 0, 0, humidity_guage_dial, sizeof(humidity_guage_dial));
		// Serial.println("Loaded PNG Graphics");
	}
	else
	{
		Serial.println(device);
	}
}
