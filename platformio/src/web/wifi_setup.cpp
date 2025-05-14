/**
 *
 * @details The `WifiSetup` class handles the setup of WiFi AP connections for the SQUiXL device. It provides an access point (AP) mode for connecting the device to a WiFi network and storing the AP credentials.
 *
 */
#include "squixl.h"
#include "web/wifi_setup.h"
#include "web/wifi_setup_templates.h"
#include <Arduino.h>
#include <WiFi.h>

static void htmlEscape(String &str)
{
	str.replace("&", "&amp;");
	str.replace("<", "&lt;");
	str.replace(">", "&gt;");
}

static bool isPortalRequest(AsyncWebServerRequest *request) { return request->host() == WiFi.softAPIP().toString(); }

struct scan_responder
{
		WifiSetup *server;
		bool waiting = true;
		std::string response;

		scan_responder(WifiSetup *server) : server(server) {}

		size_t operator()(uint8_t *buffer, size_t maxLen, size_t index)
		{
			if (waiting)
			{
				auto netCount = WiFi.scanComplete();
				if (netCount < 0)
				{
					return RESPONSE_TRY_AGAIN;
				}

				// // limit the SSID count to max 20 items to prevent captive portal crashing
				if (netCount > 16)
					netCount = 16;

				server->update_wifisetup_status(String("FOUND ") + netCount + " NETWORKS", RGB(0xff, 0xc9, 0x00));
				waiting = false;
				response = startHtml;
				// todo: sort by rssi?
				for (int i = 0; i < netCount; i++)
				{
					auto ssid = WiFi.SSID(i);
					auto rssi = WiFi.RSSI(i);
					htmlEscape(ssid);
					response += R"~(<div class="ssid_extra"><a href="#" class="ssid">)~";
					response += ssid.c_str();
					response += "</a> (";
					response += std::to_string(rssi);
					response += ")";
					response += "</div>";
				}
				response += formHtml;
				response += footerHtml;
			}
			size_t len = min(response.size(), maxLen);
			memcpy(buffer, response.c_str(), len);
			response.erase(0, len);
			return len;
		}
};

struct connect_responder
{
		WifiSetup *server;
		bool waiting = true;
		unsigned long start;
		static const int timeout = 15000;
		std::string response;

		connect_responder(WifiSetup *server) : start(millis()), server(server) {}

		size_t operator()(uint8_t *buffer, size_t maxLen, size_t index)
		{
			if (waiting)
			{
				auto status = WiFi.status();
				if (status == WL_IDLE_STATUS && millis() - start < timeout)
				{
					return RESPONSE_TRY_AGAIN;
				}
				waiting = false;
				response = startHtml;
				if (status == WL_CONNECTED)
				{
					// audio tone
					// BuzzerUI({{2093, 100}, {0, 50}, {2349, 100}, {0, 50}, {2637, 100}});
					server->update_wifisetup_status("CONNECTED, RESTARTING...", RGB(0x00, 0xff, 0x00));
					response += R"~(<h2 class="green_text">Connected</h2>)~";
					server->done = true;
				}
				else
				{
					// audio tone
					// BuzzerUI({{392, 250}, {220, 250}}); // G4, A3
					server->update_wifisetup_status("FAILED TO CONNECT", RGB(0xff, 0x00, 0x00));
					response += retryHtml;
					WiFi.enableSTA(false);
				}
			}
			size_t len = min(response.size(), maxLen);
			memcpy(buffer, response.c_str(), len);
			response.erase(0, len);
			return len;
		}
};

/**
 * @brief Start the WiFi AP Hotspot portal that allows users to select their SSID from a scanned list and enter their password
 *
 */
void WifiSetup::start()
{
	done = false;
	WiFi.mode(WIFI_AP);
	WiFi.softAP("SQUiXL");
	Serial.println("AP started");
	dnsServer.start(53, "*", WiFi.softAPIP());
	Serial.println("DNS started");
	webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", String(startHtml) + scanningHtml); })
		.setFilter(isPortalRequest);

	webServer
		.on("/connect", HTTP_GET,
			[this](AsyncWebServerRequest *request) {
				// audio tone
				// BuzzerUI({{2000, 50}});
				update_wifisetup_status("SCANNING...", RGB(0xff, 0xc9, 0x00));
				// Start an async scan
				WiFi.scanNetworks(true);
				request->send(request->beginChunkedResponse("text/html", scan_responder(this)));
			})
		.setFilter(isPortalRequest);

	webServer
		.on("/connect", HTTP_POST,
			[this](AsyncWebServerRequest *request) {
				auto ssidParam = request->getParam("ssid", true);
				auto passParam = request->getParam("pass", true);

				if (ssidParam == nullptr || passParam == nullptr)
				{
					request->redirect("/");
					return;
				}

				ssid = ssidParam->value();
				pass = passParam->value();
				// audio beep
				// BuzzerUI({{2000, 50}});
				// updateStatus("TRYING TO CONNECT...", YELLOW);
				update_wifisetup_status("CONNECTING...", RGB(0xff, 0xc9, 0x00));
				Serial.printf("Connecting to %s with password %s\n", ssid.c_str(), pass.c_str());

				WiFi.begin(ssid.c_str(), pass.c_str());

				String html = String(startHtml) + connectingHtml;
				html.replace("{ssid}", ssidParam->value());
				request->send(200, "text/html", html);

				// Non-blocking connection attempt using millis()
				unsigned long startTime = millis();
				request->onDisconnect([startTime, request, this]() mutable {
					while ((millis() - startTime) < 5000)
					{
						if (WiFi.status() == WL_CONNECTED)
						{
							Serial.println("Connected!");

							return;
						}
					}
					// Handle failure case
					Serial.printf("Connection Error # %d\n", (uint8_t)WiFi.status());
								 return; });
			})
		.setFilter(isPortalRequest);

	webServer.on("/connectresult", HTTP_GET, [this](AsyncWebServerRequest *request) { request->send(request->beginChunkedResponse("text/html", connect_responder(this))); })
		.setFilter(isPortalRequest);

	// Redirect all unhandled requests to the portal
	webServer.onNotFound([](AsyncWebServerRequest *request) { request->redirect("http://" + WiFi.softAPIP().toString()); });

	Serial.println("webServer.begin();");
	_running = true;
	webServer.begin();
}

/*
							audio.play_tone(1000, 11);
							stop(true);*/
/**
 * @brief Stop the WiFi Setup Hotspot portal
 *
 * @param restart Restart the ESP32 after stopping
 */
void WifiSetup::stop(bool restart)
{
	WiFi.mode(WIFI_OFF);
	webServer.end();
	dnsServer.stop();

	_running = false;

	if (restart)
	{
		ESP.restart();
	}
	else
	{
		// display.set_current_clock_face(true);
	}
}

/**
 * @brief Call from main loop to handle dns requests
 */
void WifiSetup::process() { dnsServer.processNextRequest(); }

WifiSetup wifiSetup;
