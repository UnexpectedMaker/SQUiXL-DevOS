#pragma once

#include "wifi_common.h"
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <functional>

class ui_screen;

class WifiSetup
{
		friend struct scan_responder;
		friend struct connect_responder;

	public:
		WifiSetup() : webServer(80) {}

		String wifi_ap_messages = "WAITING...";
		uint32_t wifi_ap_message_color;
		bool wifi_ap_changed = true;
		String cached_message = "";

		void start();
		void stop(bool restart);
		void process();

		bool running() { return _running; }

		// Successfully collected and validated wifi creds?
		bool is_done() const { return done; }
		String get_ssid() const { return ssid; }
		String get_pass() const { return pass; }

		void set_screen(ui_screen *screen) { _screen = screen; };
		ui_screen *screen() { return _screen; };

	private:
		void update_wifisetup_status(String status, uint32_t color)
		{
			wifi_ap_messages = status;
			wifi_ap_message_color = color;
			wifi_ap_changed = true;
		}

		bool _running = false;

		AsyncWebServer webServer;
		DNSServer dnsServer;
		String ssid;
		String pass;
		bool done;

		ui_screen *_screen = nullptr;
};

extern WifiSetup wifiSetup;
