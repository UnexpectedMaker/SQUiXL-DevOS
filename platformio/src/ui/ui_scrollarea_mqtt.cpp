#include "ui/ui_scrollarea_mqtt.h"

#include "mqtt/mqtt.h"

bool ui_scrollarea_mqtt::external_content_dirty() const
{
	return mqtt_stuff.mqtt_dirty;
}

void ui_scrollarea_mqtt::render_content()
{
	int line_y = char_height;
	content_height = 0;

	for (auto &sensor_data : mqtt_stuff.mqtt_topic_payloads)
	{
		for (const auto &entry : sensor_data.second)
		{
			psram_string data = entry.description + " " + entry.device_class + " " + entry.sensor_value + entry.unit_of_measurement;
			_sprite_content.setCursor(10, _scroll_y + line_y);
			_sprite_content.print(data.c_str());
			line_y += 18;
			content_height += 18;
		}
	}

	if (content_height == 0)
	{
		_sprite_content.setCursor(10, _scroll_y + char_height);
		_sprite_content.print("No MQTT messages");
		content_height = char_height + 18;
	}
}

void ui_scrollarea_mqtt::after_content_render()
{
	mqtt_stuff.mqtt_dirty = false;
}
