#include "ui/ui_scrollarea_wifimanager.h"

#include <WiFi.h>
#include "ui/ui_screen.h"
#include "ui/controls/ui_control_textbox.h"
#include "web/wifi_controller.h"
#include "audio/audio.h"

extern ui_control_textbox *text_wifimanager_ssid;

namespace
{
const char *auth_mode_to_str(wifi_auth_mode_t mode)
{
	switch (mode)
	{
	case WIFI_AUTH_OPEN:
		return "Open";
	case WIFI_AUTH_WEP:
		return "WEP";
	case WIFI_AUTH_WPA_PSK:
		return "WPA";
	case WIFI_AUTH_WPA2_PSK:
		return "WPA2";
	case WIFI_AUTH_WPA_WPA2_PSK:
		return "WPA/WPA2";
	case WIFI_AUTH_WPA2_ENTERPRISE:
		return "WPA2-Ent";
#ifdef WIFI_AUTH_WPA3_PSK
	case WIFI_AUTH_WPA3_PSK:
		return "WPA3";
#endif
#ifdef WIFI_AUTH_WPA2_WPA3_PSK
	case WIFI_AUTH_WPA2_WPA3_PSK:
		return "WPA2/WPA3";
#endif
	default:
		return "Unknown";
	}
}
} // namespace

bool ui_scrollarea_wifimanager::external_content_dirty() const
{

	return wifi_controller.is_scan_in_progress() || !wifi_controller.scan_results().empty();
}

void ui_scrollarea_wifimanager::show_connected()
{
	String current_ssid = WiFi.SSID();
	String connected_text = "Connected to: " + current_ssid;

	uint16_t text_len_pixels = connected_text.length() * char_width;

	_sprite_top.setTextColor(TFT_CYAN, -1);
	_sprite_top.setFreeFont(UbuntuMono_R[1]);
	_sprite_top.setCursor(_w - padding.right - text_len_pixels, char_height + 2);
	_sprite_top.print(connected_text.c_str());

	// Serial.println(connected_text);

	shown_connected = true;
}

void ui_scrollarea_wifimanager::render_content()
{
	if (!shown_connected && wifi_controller.is_connected())
	{
		show_connected();
	}

	_sprite_back.setCursor(padding.left, char_height + 2);
	_sprite_back.print(_title.c_str());

	squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 2, &wifi_char_width, &wifi_char_height);
	_sprite_content.setFreeFont(UbuntuMono_R[2]);

	auto *screen = static_cast<ui_screen *>(get_ui_parent());
	if (screen)
	{
		_sprite_content.setTextColor(TFT_WHITE, -1);
	}

	int line_y = char_height + 10;
	content_height = 0;

	if (wifi_controller.is_scan_in_progress())
	{
		_sprite_content.setCursor(10, _scroll_y + line_y);
		_sprite_content.print("Scanning for WiFi networks...");
		content_height = line_y + LINE_HEIGHT;
		return;
	}

	const auto &networks = wifi_controller.scan_results();
	if (networks.empty())
	{
		_sprite_content.setCursor(10, _scroll_y + line_y);
		_sprite_content.print("No WiFi networks found.");
		content_height = line_y + LINE_HEIGHT;
		return;
	}

	int item_index = 0;
	int vertical_padding = (LINE_HEIGHT - char_height) / 2;
	for (const auto &network : networks)
	{
		int row_y = _scroll_y + line_y - char_height - vertical_padding;

		// Draw highlight if this is the flashed item
		if (item_index == flash_index)
		{
			_sprite_content.fillRect(0, row_y, content_sprite_width, LINE_HEIGHT, TFT_WHITE);
			_sprite_content.setTextColor(TFT_BLACK, TFT_WHITE);
		}
		else
		{
			_sprite_content.setTextColor(TFT_WHITE, body_color);
		}

		_sprite_content.setCursor(10, _scroll_y + line_y);
		_sprite_content.print(network.name.c_str());
		// _sprite_content.print("  ");
		_sprite_content.setCursor(240, _scroll_y + line_y);
		_sprite_content.print(network.rssi);
		_sprite_content.print("dBm");

		_sprite_content.setCursor(340, _scroll_y + line_y);
		_sprite_content.print(auth_mode_to_str(network.encryption));

		line_y += LINE_HEIGHT;
		content_height += LINE_HEIGHT;
		item_index++;
	}

	if (content_height == 0)
	{
		content_height = line_y + LINE_HEIGHT;
	}
}

void ui_scrollarea_wifimanager::about_to_show_screen()
{
	is_dragging = false;
	acceleration_y = 0;

	if (!wifi_controller.is_scan_in_progress() && wifi_controller.scan_results().empty())
	{
		wifi_controller.start_async_scan();
	}
}

void ui_scrollarea_wifimanager::start_rescan()
{
	shown_connected = false;
	content_changed = true;
	wifi_controller.start_async_scan();
}

bool ui_scrollarea_wifimanager::process_touch(touch_event_t touch_event)
{
	// Handle tap to select a network
	if (touch_event.type == TOUCH_TAP)
	{
		// Check if tap is within content area (excluding header and borders)
		int content_x = touch_event.x - _x - padding.left;
		int content_y = touch_event.y - _y - 26; // 26px header

		if (content_x >= 0 && content_x < content_sprite_width &&
			content_y >= 0 && content_y < content_sprite_height)
		{
			// Account for scroll position (_scroll_y is negative when scrolled down)
			int actual_y = content_y - _scroll_y;

			// Each item row is LINE_HEIGHT tall, starting at y=0
			int item_index = actual_y / LINE_HEIGHT;

			const auto &networks = wifi_controller.scan_results();
			if (item_index >= 0 && item_index < (int)networks.size())
			{
				// Visual feedback - flash the selected line
				flash_index = item_index;
				content_changed = true;
				redraw(32);
				if (auto *screen = static_cast<ui_screen *>(get_ui_parent()))
					screen->refresh(true);

				// Audio feedback
				audio.play_tone(500, 1);

				delay(100);

				flash_index = -1;
				content_changed = true;
				redraw(32);
				if (auto *screen = static_cast<ui_screen *>(get_ui_parent()))
					screen->refresh(true);

				// Set the SSID text
				if (text_wifimanager_ssid != nullptr)
				{
					text_wifimanager_ssid->set_text(networks[item_index].name.c_str());
					text_wifimanager_ssid->redraw(32);
				}
				return true;
			}
		}
	}

	// Let base class handle scrolling
	return ui_scrollarea::process_touch(touch_event);
}
