#include "mqtt/mqtt.h"
#include <PubSubClient.h>

using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MQTT_Payload, owner, device_class, state_class, unit_of_measurement, sensor_value, value_type, min_value, max_value, description, timestamp);

WiFiClient espClientMQTT;
PubSubClient mqtt_client(espClientMQTT);

// Static callback that wraps the instance method
void MQTT_Stuff::static_mqtt_callback(char *topic, byte *message, unsigned int length)
{
	mqtt_stuff.mqtt_callback(topic, message, length);
}

void MQTT_Stuff::mqtt_callback(char *topic, byte *message, unsigned int length)
{
	String messageTemp;
	for (int i = 0; i < length; i++)
		messageTemp += (char)message[i];

	Serial.printf("New message on topic: %s\n", topic);
	Serial.printf("message: %s\n", messageTemp.c_str());

	// If the string "sensor" is in the topic, we can assume it's sensor data coming in
	if (strstr(topic, "sensor") != NULL)
	{
		// Parse the string into a Json structure
		json json_data = json::parse(messageTemp);
		// Convert json to struct
		MQTT_Payload payload = json_data.get<MQTT_Payload>();

		mqtt_dirty = false;

		if (mqtt_topic_payloads.find(payload.owner) == mqtt_topic_payloads.end())
		{
			mqtt_topic_payloads[payload.owner].push_back(payload);
			Serial.printf("\nMQTT: New Sensor owner: %s\nAdded new %s sensor at %s\n", payload.owner.c_str(), payload.device_class.c_str(), payload.description.c_str());
			mqtt_dirty = true;
		}
		else
		{
			for (int s = 0; s < mqtt_topic_payloads[payload.owner].size(); s++)
			{
				if (mqtt_topic_payloads[payload.owner][s].device_class == payload.device_class && mqtt_topic_payloads[payload.owner][s].timestamp < payload.timestamp)
				{
					mqtt_topic_payloads[payload.owner][s].upate_from(payload);
					Serial.printf("\nMQTT: Updated sensor %s for %s\n", payload.device_class.c_str(), payload.owner.c_str());
					mqtt_dirty = true;
					break;
				}
			}
		}

		if (!mqtt_dirty)
		{
			mqtt_topic_payloads[payload.owner].push_back(payload);
			Serial.printf("\nMQTT: Added new sensor %s for %s at %s\nNow has %d sensors\n", payload.device_class.c_str(), payload.owner.c_str(), payload.description.c_str(), mqtt_topic_payloads[payload.owner].size());

			mqtt_dirty = true;
		}
	}
}

void MQTT_Stuff::mqtt_reconnect()
{
	is_mqtt_connecting = true;

	if (!mqtt_server_setup)
	{
		Serial.println("\nAttempting MQTT Server Setup..");
		mqtt_server_setup = true;
		mqtt_client.setServer(settings.config.mqtt.broker_ip.c_str(), 1883);
		// mqtt_client.setServer(mqtt_server, 1883);
		mqtt_client.setCallback(MQTT_Stuff::static_mqtt_callback);
		mqtt_client.setBufferSize(512);
		is_mqtt_connecting = false;
		next_mqtt_reconnect = millis() + 5000;
		return;
	}

	Serial.print("\nMQTT: Attempting connection to ");
	Serial.println(settings.config.mqtt.broker_ip);
	// Serial.print("My IP address is ");
	// Serial.println(WiFi.localIP());
	// Serial.println();
	// Attempt to connect
	if (mqtt_client.connect(settings.config.mqtt.device_name.c_str(), settings.config.mqtt.username.c_str(), settings.config.mqtt.password.c_str()))
	{
		Serial.println("MQTT: connected");
		// Subscribe
		for (int i = 0; i < settings.config.mqtt.topics.size(); i++)
		{
			if (!settings.config.mqtt.topics[i].topic_listen.isEmpty())
			{
				Serial.printf("MQTT: Subscribing to topic: %s\n", settings.config.mqtt.topics[i].topic_listen.c_str());
				mqtt_client.subscribe(settings.config.mqtt.topics[i].topic_listen.c_str(), 1);
			}
		}

		is_mqtt_connecting = false;
	}
	else
	{

		retry_attemps--;
		retry_time += 5000;

		Serial.print("MQTT: connection failed, rc=");
		Serial.print(mqtt_client.state());
		if (retry_attemps >= 0)
			Serial.printf(" try again (attempt %d/3) in %d seconds\n", (4 - retry_attemps), (retry_time / 1000));
		else
			Serial.println("\nMQTT: No more attempts to cpnect to MQTT!");

		// Wait 5 seconds before retrying

		is_mqtt_connecting = false;
	}
}

void MQTT_Stuff::process_mqtt()
{
	if (millis() - next_mqtt_reconnect > retry_time && !mqtt_client.connected())
	{
		next_mqtt_reconnect = millis();

		if (!is_mqtt_connecting && retry_attemps > 0)
			mqtt_reconnect();
	}
	else
	{
		mqtt_client.loop();
	}
}

MQTT_Stuff mqtt_stuff;
