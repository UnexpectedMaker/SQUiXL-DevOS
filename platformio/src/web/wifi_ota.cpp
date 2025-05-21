#include "web/wifi_ota.h"
#include "settings/settings.h"

void start_ota()
{
	ArduinoOTA.setHostname(settings.config.mdns_name.c_str());
	ArduinoOTA
		.onStart([]() {
			is_ota_updating = true;

			String type;
			if (ArduinoOTA.getCommand() == U_FLASH)
				type = "sketch";
			else // U_SPIFFS
				type = "filesystem";

			// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
			// using SPIFFS.end()
			Serial.println("Start updating " + type);
			is_ota_updating = true;
		})
		.onEnd([]() {
			Serial.println("\nEnd");
			is_ota_updating = false;
		})
		.onProgress([](unsigned int progress, unsigned int total) {

		})
		.onError([](ota_error_t error) {
			Serial.printf("Error[%u]: ", error);
			if (error == OTA_AUTH_ERROR)
				Serial.println("Auth Failed");
			else if (error == OTA_BEGIN_ERROR)
				Serial.println("Begin Failed");
			else if (error == OTA_CONNECT_ERROR)
				Serial.println("Connect Failed");
			else if (error == OTA_RECEIVE_ERROR)
				Serial.println("Receive Failed");
			else if (error == OTA_END_ERROR)
				Serial.println("End Failed");
			is_ota_updating = false;
		});

	ArduinoOTA.begin();
}