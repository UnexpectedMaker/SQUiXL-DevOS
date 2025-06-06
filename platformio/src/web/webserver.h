#pragma once

#include "wifi_common.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "settings/settings_async.h"
#include <functional>

class WebServer
{

		struct BufferRequestCtx
		{
				AsyncWebServerRequest *request;
				// Optionally: any other info you want to store
		};

	public:
		WebServer() : web_server(80), web_event("/event") {}

		void start();
		void start_callback(bool success, const String &response);
		void stop(bool restart);
		void process();
		bool is_running();

		static void do_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

		static String processor(const String &var);

		static String generate_settings_html(int group_id);
		static String generate_themes_html(int group_id);

		SettingsOptionBase *get_obj_from_id(String id);

		AsyncEventSource web_event;

	private:
		bool _running = false;
		AsyncWebServer web_server;
		DNSServer dns_server;
};

extern WebServer webserver;
