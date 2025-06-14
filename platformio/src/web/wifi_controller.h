#pragma once

#include "HTTPClient.h"
#include "utils/json.h"
#include "utils/json_conversions.h"
#include "web/wifi_common.h"
#include <freertos/queue.h>
#include <functional>
#include <string>

#include <queue>
#include <map>
#include <freertos/semphr.h>

typedef std::function<void(bool, const String &)> _CALLBACK;

class WifiController
{
	public:
		WifiController();

		bool connect();
		void disconnect(bool force);
		bool is_busy();
		void kill_controller_task();
		bool is_connected();

		// task queue related functions
		void perform_wifi_request(std::string, _CALLBACK callback);
		void add_to_queue(std::string, _CALLBACK callback);
		void loop();

		String http_request(std::string url);

		bool wifi_blocking_access = false;
		bool wifi_prevent_disconnect = false;

		uint8_t items_in_queue() { return queue_size; }

	private:
		String user_config_json;
		wifi_states current_state = BOOT;
		bool wifi_busy = false;
		unsigned long next_wifi_loop = 0;
		uint8_t queue_size = 0;
		uint8_t download_error_count = 0;

		// Structure for task items
		struct wifi_task_item
		{
				std::string url;
				_CALLBACK callback;
		};

		// Structure for callback items
		struct wifi_callback_item
		{
				bool success;
				String *response;
				_CALLBACK callback;
		};

		// Maybe a way to cache DNS lookups?
		struct request_dns
		{
				std::string domain;
				IPAddress ip;
		};

		TaskHandle_t wifi_task_handler;
		QueueHandle_t wifi_task_queue;
		QueueHandle_t wifi_callback_queue;

		std::queue<wifi_task_item *> pending_requests;
		SemaphoreHandle_t pending_mutex = nullptr;

		static void wifi_task(void *pvParameters);

		std::map<std::string, request_dns> dns_cache;
		std::string extract_domain(const std::string &url);
};

extern WifiController wifi_controller;