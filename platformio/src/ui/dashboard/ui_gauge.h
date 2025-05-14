#pragma once

#include "ui/ui_element.h"
#include "ui/icons/images/dashboard/dashboard_graphics.h"

struct MQTT_Payload;

class ui_gauge : public ui_element
{
	public:
		void create(int16_t x, int16_t y, int16_t w, int16_t h);
		void set_payload_index(const char *key, uint16_t index, const char *device);

		void load_graphics(const char *device);

	protected:
		uint16_t _payload_index = 0;
		const char *_payload_key;

		int cached_device_text_w = 0;
		int cached_device_text_h = 0;

		int cached_desc_text_w = 0;
		int cached_desc_text_h = 0;

		BB_SPI_LCD _graphic_back;
		BB_SPI_LCD _graphic_front;
};