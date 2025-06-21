/**
 * @file wifi_controller.cpp
 * @details `WifiController` is a class designed for managing non-blocking WiFi connectivity and HTTP request handling in a separate thead using and incoming task queue and outgoing callback queue.

 The idea behind this class is to implement a way to fire off a HTTP or HTTPS request along with a callback, and have the WiFi connection, request and disconnection happen without blocking the main thread.
 *
 */
#include "web/wifi_controller.h"
#include "settings/settings_async.h"

using json = nlohmann::json;

// Initialise the controller, create the incoming and outgoing queues and start the process task
WifiController::WifiController()
{
	// Create the task and callback queues
	wifi_task_queue = xQueueCreate(10, sizeof(wifi_task_item *));
	wifi_callback_queue = xQueueCreate(10, sizeof(wifi_callback_item));

	if (wifi_task_queue == NULL || wifi_callback_queue == NULL)
	{
		Serial.println("Error creating the queues");
		return;
	}

	pending_mutex = xSemaphoreCreateMutex();
	if (pending_mutex == nullptr)
	{
		Serial.println("Error creating pending_requests mutex");
		// handle error if you want
	}

	// Start the WiFi task
	// xTaskCreate(WifiController::wifi_task, "wifi_task", 8192 * 2, this, 2, &wifi_task_handler);
	xTaskCreatePinnedToCore(WifiController::wifi_task, "wifi_task", 8192 * 2, this, 3, &wifi_task_handler, 0);

	wifi_prevent_disconnect = true;

	WiFi.mode(WIFI_OFF);
}

// Kill the pinned threaded task
void WifiController::kill_controller_task()
{
	Serial.println("Killing WiFi queue task!");
	vTaskDelete(wifi_task_handler);
	vQueueDelete(wifi_task_queue);
	vQueueDelete(wifi_callback_queue);

	if (pending_mutex)
		vSemaphoreDelete(pending_mutex);
}

// Return the busy state of the WiFi queue
bool WifiController::is_busy() { return wifi_busy; }

bool WifiController::is_connected() { return (WiFi.status() == WL_CONNECTED); }

// Connect to the WiFi network
bool WifiController::connect()
{
	// Serial.println("Conecting to wifi");
	if (WiFi.status() == WL_CONNECTED)
	{
		// Serial.println("Already connected to Wifi");
		wifi_busy = false;
		return true;
	}

	uint8_t start_index = settings.config.current_wifi_station;

	if (!settings.has_wifi_creds())
	{
		// Serial.println("No credentials?");
		wifi_busy = false;
		return false;
	}
	else
	{
		Serial.println("WIFI: Attempting to connect....");
		wifi_busy = true;
		// if (WiFi.disconnect(true))
		// {
		// 	Serial.println("WIFI: Disconnected before re-connect attempt..");
		// 	delay(100);
		// }
		if (WiFi.mode(WIFI_STA))
		{
			Serial.println("WIFI: Mode set to STA");
			delay(100);
		}
		// delay(500);
		uint8_t stations_to_try = settings.config.wifi_options.size();
		// Serial.printf("Trying wifi index %d - %s %s\n", settings.config.current_wifi_station, settings.config.wifi_options[settings.config.current_wifi_station].ssid, settings.config.wifi_options[settings.config.current_wifi_station].pass);
		// WiFi.begin(settings.config.wifi_options[settings.config.current_wifi_station].ssid, settings.config.wifi_options[settings.config.current_wifi_station].pass);

		// Ensure we are using DHCP settings if no local DNS is selected.
		if (!settings.config.use_local_dns)
			WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);

		while (stations_to_try > 0)
		{
			// Serial.printf("Trying wifi index %d - %s %s\n", settings.config.current_wifi_station, settings.config.wifi_options[settings.config.current_wifi_station].ssid.c_str(), settings.config.wifi_options[settings.config.current_wifi_station].pass.c_str());

			WiFi.begin(settings.config.wifi_options[settings.config.current_wifi_station].ssid.c_str(), settings.config.wifi_options[settings.config.current_wifi_station].pass.c_str());

			// delay(100);
			unsigned long start_time = millis();
			// Time out the connection if it takes longer than 45 seconds
			while ((millis() - start_time < 5000) && WiFi.status() != WL_CONNECTED)
			{
				delay(100);
			}

			if (WiFi.status() != WL_CONNECTED)
			{
				stations_to_try--;

				settings.config.current_wifi_station++;
				if (settings.config.current_wifi_station == settings.config.wifi_options.size())
					settings.config.current_wifi_station = 0;

				Serial.printf("SQUiXL WiFI: Unable to connect to WiFi Router.\nResponse WiFi.status() code: %d, Saved stations left to try: %d/%d\n", WiFi.status(), stations_to_try, settings.config.wifi_options.size());
			}
			else
			{

				break;
			}
			delay(500);
		}
	}

	delay(100);

	Serial.printf("wifi status %d\n\n", WiFi.status());

	if (WiFi.status() == WL_CONNECTED)
	{
		// Serial.printf("SQUiXL WiFI: Connected to WiFi Router using index %d - %s %s\n", settings.config.current_wifi_station, settings.config.wifi_options[settings.config.current_wifi_station].ssid.c_str(), settings.config.wifi_options[settings.config.current_wifi_station].pass.c_str());

		Serial.println("WIFI: Connected");

		if (settings.config.use_local_dns)
		{
			WiFi.setDNS(IPAddress(1, 1, 1, 1), IPAddress(8, 8, 8, 8));
			Serial.print("Using Local DNS - Primary: ");
		}
		else
		{
			Serial.print("Using DHCP DNS - Primary: ");
		}
		Serial.print(WiFi.dnsIP(0));
		Serial.print(", Secondary: ");
		Serial.println(WiFi.dnsIP(1));

		// If we are connected and it's on a different network than last time, we save the settings with the new connection index
		if (settings.config.current_wifi_station != start_index)
			settings.save(true);

		// Serial.print("IP Address: ");
		// Serial.println(WiFi.localIP());
		WiFi.setHostname(settings.config.mdns_name.c_str());
	}
	else
	{
		// WiFi.disconnect(true);
		Serial.println("WiFI: Was unable to connect SQUiXL to the WiFi Router. Too bad, so sad :(");
	}

	wifi_busy = false;

	return (WiFi.status() == WL_CONNECTED);
}

// Disconnect from the WiFi network
void WifiController::disconnect(bool force)
{
	Serial.println("wifi disconnect, forced? " + String(force));
	if (!wifi_prevent_disconnect || force)
	{
		WiFi.disconnect(true);
		// WiFi.mode(WIFI_OFF);
	}
	wifi_busy = false;
}

// Process the task queue - called from the main thread in loop() in squixl.cpp
// only process every 2 seconds
// void WifiController::loop()
// {
// 	if (millis() - next_wifi_loop > 2000)
// 	{
// 		next_wifi_loop = millis();
// 		wifi_callback_item result;
// 		if (xQueueReceive(wifi_callback_queue, &result, 0) == pdTRUE)
// 		{
// 			result.callback(result.success, *result.response);
// 			delete result.response;
// 		}

// 		xSemaphoreTake(pending_mutex, portMAX_DELAY);
// 		if (!pending_requests.empty())
// 		{
// 			wifi_task_item *item = pending_requests.front();
// 			pending_requests.pop();

// 			perform_wifi_request(item->url, item->callback);
// 			delete item;
// 		}
// 		xSemaphoreGive(pending_mutex);
// 	}
// }

void WifiController::loop()
{
	if (millis() - next_wifi_loop > 2000)
	{
		next_wifi_loop = millis();
		wifi_callback_item result;
		while (xQueueReceive(wifi_callback_queue, &result, 0) == pdTRUE)
		{
			result.callback(result.success, *result.response);
			// delete result.response;
		}
	}
}

// Make an HTTP request and return the result as a String
String WifiController::http_request(std::string url)
{
	String payload = "ERROR";
	int http_code = -1;
	String url_lower = String(url.c_str());
	url_lower.toLowerCase();

	bool is_https = (url_lower.substring(0, 5) == "https");
	std::string domain = extract_domain(url);

	// Only resolve/cache/use IP for HTTP
	IPAddress resolved_ip;
	bool ip_cached = false;

	// For HTTP: try cache and/or resolve, else bail out
	if (!is_https)
	{
		auto it = dns_cache.find(domain);
		if (it != dns_cache.end())
		{
			resolved_ip = it->second.ip;
			ip_cached = true;
			Serial.printf("Using cached DNS for %s - %d.%d.%d.%d\n", domain.c_str(), resolved_ip[0], resolved_ip[1], resolved_ip[2], resolved_ip[3]);
		}
		else
		{
			if (WiFi.hostByName(domain.c_str(), resolved_ip) == 1)
			{
				dns_cache[domain] = {domain, resolved_ip};
				ip_cached = true;
				Serial.printf("Cached DNS for %s as %d.%d.%d.%d\n", domain.c_str(), resolved_ip[0], resolved_ip[1], resolved_ip[2], resolved_ip[3]);
			}
			else
			{
				Serial.printf("DNS Failed for '%s'\n", domain.c_str());
				Serial.println("No cached IP, aborting");
				return payload;
			}
		}
	}

	// --- HTTP request ---
	HTTPClient http;
	http.setTimeout(5000);
	if (is_https)
	{
		Serial.printf("HTTPS request: %s\n", url.c_str());
		http.begin(url.c_str());
	}
	else
	{
		WiFiClient client;
		std::string url_with_ip = url;

		// Always use the cached IP for HTTP (if we have it, and at this point we always do or would have returned)
		size_t pos = url_with_ip.find(domain);
		if (pos != std::string::npos)
			url_with_ip.replace(pos, domain.length(), resolved_ip.toString().c_str());

		Serial.printf("HTTP IP request: %s\n", url_with_ip.c_str());
		http.begin(client, url_with_ip.c_str());
		http.addHeader("Host", domain.c_str());
	}

	http_code = http.GET();

	if (http_code != 200)
	{
		Serial.println("** Response Code: " + String(http_code));
		http.end();
	}
	else
	{
		payload = http.getString();
		http.end();
	}

	return payload;
}

std::string WifiController::extract_domain(const std::string &url)
{
	size_t start = url.find("://");
	if (start == std::string::npos)
		start = 0;
	else
		start += 3;
	size_t end = url.find('/', start);
	return url.substr(start, end - start);
}

// Function to call out to the HTTP Request and then add the result to the outgoing queue
void WifiController::perform_wifi_request(std::string url, _CALLBACK callback)
{
	bool success = true;
	String response = "OK";

	// Only process if there is an actual URL, otherwise do the callback
	if (!url.empty())
	{
		response = http_request(url);
		success = (response != "ERROR"); // or false, based on the HTTP request result
	}

	if (!success)
	{
		download_error_count++;
		Serial.printf("WIFI Download Error Count: %d\n", download_error_count);

		if (is_connected() && download_error_count == 5)
		{
			download_error_count = 0;
			disconnect(true);
		}
	}

	// Create a wifi_callback_item and enqueue it
	wifi_callback_item result = {success, new String(response), callback};
	xQueueSend(wifi_callback_queue, &result, portMAX_DELAY);
}

// Task for processing the queue items
void WifiController::wifi_task(void *pvParameters)
{
	WifiController *controller = static_cast<WifiController *>(pvParameters);
	while (true)
	{
		wifi_task_item *item = nullptr;
		if (xQueueReceive(controller->wifi_task_queue, &item, portMAX_DELAY) == pdTRUE)
		{
			// Connect to WiFi / pass through if already connected
			if (controller->connect())
			{
				controller->wifi_busy = true;
				// Perform the request
				controller->perform_wifi_request(item->url, item->callback);

				// Wait until callback queue is empty before processing the next task
				while (uxQueueMessagesWaiting(controller->wifi_callback_queue) > 0)
				{
					// Serial.print("~");
					vTaskDelay(pdMS_TO_TICKS(10)); // Wait for callbacks to be processed
				}

				// Clean up once done:
				delete item;
				controller->wifi_busy = false;

				vTaskDelay(100);
			}
		}
	}
}

// Function to add items to the queue
void WifiController::add_to_queue(std::string url, _CALLBACK callback)
{
	wifi_task_item *item = new wifi_task_item;

	item->url = url;
	item->callback = callback;

	xQueueSend(wifi_task_queue, &item, portMAX_DELAY);

	// Serial.printf("\nHeap Log: WIFI\nHeap Size: %u of %u\n", ESP.getFreeHeap(), ESP.getHeapSize());
	// Serial.printf("Min Heap Size: %u, Max Alloc Heap Size: %u, ", ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
	// Serial.printf("PSRAM Size: %u\n\n", ESP.getFreePsram());
}

WifiController wifi_controller;
