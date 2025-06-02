#include "squixl.h"
#include "web/webserver.h"
#include "peripherals/haptics.h"
#include "settings/settings.h"
#include "web/wifi_controller.h"
#include "ui/ui_screen.h"

// HTML Templates
#include "web/www/www_general.h"
#include "web/www/www_settings_main.h"

static String file_size_helper(const size_t bytes)
{
	if (bytes < 1024)
		return String(bytes) + " B";
	else if (bytes < (1024 * 1024))
		return String(bytes / 1024.0) + " KB";
	else
		return String(bytes / 1024.0 / 1024.0) + " MB";
}

static String file_system_list(bool ishtml)
{

	// LittleFS.remove("/IMG_2614.jpg");
	// LittleFS.remove("/wallpaper_example.jpg");

	String returnText = "";
	Serial.println("Listing files stored on LittleFS");
	File root = LittleFS.open("/");
	File foundfile = root.openNextFile();

	if (ishtml)
	{
		returnText += "<table style='font-size:20px;'><tr><th align='left'>Name</th><th align='left'>Size</th></tr>";
	}
	while (foundfile)
	{
		if (ishtml)
		{
			returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + file_size_helper(foundfile.size()) + "</td></tr>";
		}
		else
		{
			returnText += "File: " + String(foundfile.name()) + "\n";
		}

		foundfile = root.openNextFile();
	}
	if (ishtml)
	{
		returnText += "</table>";
	}
	root.close();
	foundfile.close();
	return returnText;
}

String WebServer::processor(const String &var)
{
	if (var == "META")
	{
		return String(meta);
	}
	if (var == "FOOTER")
	{
		return String(footer);
	}
	else if (var == "FOOTER_WALLPAPER")
	{
		return String(footer_wallpaper);
	}
	else if (var == "THEME")
	{
		return "dark";
	}
	else if (var == "CSS")
	{
		return String(css_dark);
	}
	else if (var == "FW_VER")
	{
		return (squixl.version_firmware + " " + squixl.version_year);
	}
	else if (var == "UPDATE_NOTICE")
	{
		if (squixl.update_available())
			return "<a href='https://squixl.io/up/' target='_blank'><h2 style='padding:2px;'>NEW VERSION AVAILABLE</h2></a>\n";
		else
			return "";
	}
	else if (var == "FOOTER_STYLE")
	{
		if (squixl.update_available())
			return " style='height:100px;'";
		else
			return "";
	}

	else if (var == "DEBUG_LOGS")
	{
		String logs = "";
		// for (size_t l = 0; l < squixl.messages.size(); l++)
		// {
		// 	String mess = squixl.messages[l].message;
		// 	mess.replace(" %:", " %%:");
		// 	logs += "<div class='row p-1 " + String(l % 2 == 0 ? "alt" : "alt2") + "'>\n";
		// 	logs += "<div class='col-2 text-end nowrap'>" + squixl.messages[l].get_time() + "</div>\n";
		// 	logs += "<div class='col-10 text-start'>" + mess + "</div>\n";
		// 	logs += "</div>\n";
		// }

		return logs;
	}
	else if (var == "SETTING_OPTIONS_MAIN")
	{
		String html = "";
		for (size_t i = 0; i < settings.settings_groups.size(); i++)
		{
			if (settings.settings_groups[i].type == SettingType::MAIN)
				html += generate_settings_html(i);
		}

		return html;
	}
	else if (var == "SETTING_SCREENIE")
	{
		String html = "";
		for (size_t i = 0; i < settings.settings_groups.size(); i++)
		{
			if (settings.settings_groups[i].type == SettingType::SCREENIE)
				html += generate_settings_html(i);
		}

		return html;
	}
	else if (var == "SETTING_OPTIONS_WEB")
	{
		String html = "";
		for (size_t i = 0; i < settings.settings_groups.size(); i++)
		{
			if (settings.settings_groups[i].type == SettingType::WEB)
				html += generate_settings_html(i);
		}

		return html;
	}
	else if (var == "SETTING_OPTIONS_WIDGETS")
	{
		String html = "";
		for (size_t i = 0; i < settings.settings_groups.size(); i++)
		{
			if (settings.settings_groups[i].type == SettingType::WIDGET)
				html += generate_settings_html(i);
		}

		return html;
	}
	else if (var == "SETTING_OPTIONS_THEMES")
	{
		return "Soon!";

		String html = "";
		for (size_t i = 0; i < settings.settings_groups.size(); i++)
		{
			if (settings.settings_groups[i].type == SettingType::THEME)
				html += generate_themes_html(i);
		}

		return html;
	}
	else if (var == "FILELIST")
	{
		return file_system_list(true);
	}
	else if (var == "LFS_FREE")
	{
		return file_size_helper((LittleFS.totalBytes() - LittleFS.usedBytes()));
	}

	else if (var == "LFS_USED")
	{
		return file_size_helper(LittleFS.usedBytes());
	}

	else if (var == "LFS_TOTAL")
	{
		return file_size_helper(LittleFS.totalBytes());
	}

	return "";
}

String WebServer::generate_settings_html(int group_id)
{
	auto &group_name = settings.settings_groups[group_id]; // Cache the current group

	String group_id_str = String(group_id);

	String html = "\n<div id='settings_group_" + group_id_str + "'>\n";
	html += "	<span class='settings_heading'>" + String(group_name.name.c_str()) + "</span>\n";
	html += "	<div class='settings_frame' id='group_" + group_id_str + "' style='margin-bottom:15px; padding-bottom:5px;'>\n";

	if (group_name.description != "")
	{
		html += "		<div class='center w-100 mt-1 mb-2'>\n";
		html += "			<span class='settings_info'>" + String(group_name.description.c_str()) + +"</span>\n";
		html += "		</div>\n";
	}

	html += "		<form hx-post='/update_settings_group' hx-target='#settings_group_" + group_id_str + "' >\n";
	html += "			<input type='hidden' name='group_id' id='group_id' value='" + group_id_str + "'>\n";
	html += "			<div class ='row'>\n";

	for (size_t i = 0; i < group_name.groups.size(); ++i)
	{
		if (group_name.groups[i]->req_full_width)
			html += "				<div class='col-12 pb-1'>\n";
		else
			html += "				<div class='col-6 pb-1'>\n";
		html += group_name.groups[i]->generate_html(i);
		html += "				</div>\n\r\n";
	}
	html += "			</div>\n";

	html += "			<div class ='row'>\n";
	html += "				<div class='col-12 right align-middle' style='height:36px;'>\n";
	html += "					<span class='flash-span me-2' style='display:none; color:green;'>Settings Updated!</span>\n";
	if (group_id == 6)
		html += "					<button type='submit' class='btn btn-sm btn-success m-1' style='width:300px;'>Update & Retake Screenshot</button>\n";
	else
		html += "					<button type='submit' class='btn btn-sm btn-success m-1' style='width:100px;'>Update</button>\n";
	html += "				</div>\n";
	html += "			</div>\n";
	html += "		</form>\n";
	html += "	</div>\n";
	html += "</div>\n";
	html += "\r\n\r\n";

	return html;
}

String WebServer::generate_themes_html(int group_id)
{
	auto &group_name = settings.settings_groups[group_id]; // Cache the current group

	String group_id_str = String(group_id);

	String html = "\n<div id='settings_group_" + group_id_str + "'>\n";
	html += "	<span class='settings_heading'>" + String(group_name.name.c_str()) + "</span>\n";
	html += "	<div class='settings_frame' id='group_" + group_id_str + "' style='margin-bottom:15px; padding-bottom:5px;'>\n";

	if (group_name.description != "")
	{
		html += "		<div class='center w-100 mt-1 mb-2'>\n";
		html += "			<span class='settings_info'>" + String(group_name.description.c_str()) + "</span>\n";
		html += "		</div>\n";
	}

	html += "			<div class ='row'>\n";

	for (size_t i = 0; i < group_name.groups.size(); ++i)
	{
		if (group_name.groups[i]->req_full_width)
			html += "				<div class='col-12 pb-1'>\n";
		else
			html += "				<div class='col-6 pb-1'>\n";
		html += group_name.groups[i]->generate_html(i);
		html += "				</div>\n";
	}
	html += "			</div>\n";

	html += "			<div class ='row m-2 center'>\n";

	html += "			<div class='row' id='add_new_theme' style='display:none;'>\n";
	html += "				<span class='settings_heading'>Add New Theme</span>\n";
	html += "				<div class='settings_frame_inner' style='margin-bottom:15px; padding-bottom:5px;'>\n";
	html += "				<form hx-post='/add_theme'>\n";
	html += "				<div class='row'>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Theme Name</span>\n";
	html += "							<input type='text' class='form-control form-control-sm' id='_name' name='_name' value='' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Widget Style</span>\n";
	html += "							<input type='number' class='form-control form-control-sm' id='_widget_style' name='_widget_style' value='0' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "				</div>\n";
	html += "				<div class='row'>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Background Dull</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_background_dull' name='_col_background_dull' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Background Bright</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_background_bright' name='_col_background_bright' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "				</div>\n";
	html += "				<div class='row'>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Control Back</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_control_back' name='_col_control_back' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Low Intensity</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_low_intensity' name='_col_low_intensity' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "				</div>\n";
	html += "				<div class='row'>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Primary</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_primary' name='_col_primary' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Secondary</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_secondary' name='_col_secondary' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "				</div>\n";
	html += "				<div class='row'>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Warning</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_warning' name='_col_warning' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "					<div class='col-6 p-2'>\n";
	html += "						<div class='input-group input-group-sm'>\n";
	html += "							<span class='input-group-text' id='inputGroup-sizing-sm' style='width:150px;'>Error</span>\n";
	html += "							<input type='color' class='form-control form-control-color' id='_col_error' name='_col_error' value='#222222' />\n";
	html += "						</div>\n";
	html += "					</div>\n";
	html += "				</div>\n";
	html += "				<div class='row'>\n";

	html += "					<div class='col-6 align-middle' style='height:36px;'>\n";
	html += "						<button type='button' class='btn btn-sm btn-primary m-1' onclick='toggle_new_theme(false);' style='width:100px;'>Cancel</button>\n";
	html += "					</div>\n";
	html += "					<div class='col-6 right align-middle' style='height:36px;'>\n";
	html += "						<button type='submit' class='btn btn-sm btn-success m-1' style='width:100px;'>Save</button>\n";
	html += "					</div>\n";
	html += "				</div>\n";
	html += "				</form>\n";
	html += "				</div>\n";
	html += "			</div>\n";

	html += "			</div>\n";

	html += "			<div class ='row' id='add_new_theme_button' style='display:block;'>\n";
	html += "				<div class='col-6 align-middle' style='height:36px;'>\n";
	html += "					<button type='button' class='btn btn-sm btn-success m-1' onclick='toggle_new_theme(true);' style='width:100px;'>Add Theme</button>\n";
	html += "				</div>\n";
	html += "			</div>\n";
	html += "		</div>\n";
	html += "	</div>\n";

	return html;
}

SettingsOptionBase *WebServer::get_obj_from_id(String id)
{
	// Split the string at "__"
	int pos = id.indexOf("__");
	String firstPart = id.substring(0, pos);

	// Split the first part at ","
	int commaIndex = firstPart.indexOf(",");
	int group = firstPart.substring(0, commaIndex).toInt();
	int index = firstPart.substring(commaIndex + 1).toInt();

	return (settings.settings_groups[group].groups[index]);
}

void WebServer::start()
{
	// setCpuFrequencyMhz(80);

	_running = true;
	wifi_controller.wifi_prevent_disconnect = true;

	Serial.println("Starting webserver");

	// Start the webserver by connecting to the wifi network first, done via a non-blocking callback
	wifi_controller.add_to_queue("", [this](bool success, const String &response) { this->start_callback(success, response); });
}

void WebServer::start_callback(bool success, const String &response)
{
	if (response == "OK")
	{
		Serial.print("IP Address: ");
		Serial.println(WiFi.localIP());

		// Set the local mDSN name so you can navigate to tinywatchs3.local instead of IP address
		settings.config.mdns_name.trim();
		if (settings.config.mdns_name.isEmpty())
			settings.config.mdns_name = "SQUiXL";

		if (!MDNS.begin(settings.config.mdns_name.c_str()))
		{
			Serial.println("Error starting mDNS");
			wifi_controller.disconnect(false);
			WiFi.mode(WIFI_OFF);
			_running = false;
			return;
		}

		web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_html, processor); });

		web_server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_wifi_html, processor); });

		web_server.on("/widgets", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_widgets_html, processor); });

		web_server.on("/screenie", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", screenie_html, processor); });

		web_server.on("/wallpaper", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", wallpaper_html, processor); });

		web_server.on("/take_screenshot", HTTP_GET, [](AsyncWebServerRequest *request) {
			squixl.delayed_take_screenshot();
			request->send(200, "text/text", "Working....");
		});

		web_server.on("/screenshot", HTTP_GET, [](AsyncWebServerRequest *request) {
			// save_png(&squixl.lcd);
			request->send(LittleFS, "/screenshot.png", "image/png");
		});

		web_server.on("/get_wallpaper", HTTP_GET, [](AsyncWebServerRequest *request) {
			// save_png(&squixl.lcd);
			request->send(LittleFS, "/user_wallpaper.jpg", "image/jpg");
		});

		web_server.on("/upload_wallpaper", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); }, do_upload);

		web_event.onConnect([](AsyncEventSourceClient *client) {
			Serial.printf("SSE Client connected! ID: %" PRIu32 "\n", client->lastId());
			client->send("hello!", NULL, millis(), 1000);
		});

		web_event.onDisconnect([](AsyncEventSourceClient *client) {
			Serial.printf("SSE Client disconnected! ID: %" PRIu32 "\n", client->lastId());
		});

		// web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(404, "text/plain", "Hello SQUiXL!"); });

		// web_server.on("/index.htm", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_html, processor); });

		// web_server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_html, processor); });

		// web_server.on("/web_settings_apps.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_settings_apps_html, processor); });

		// web_server.on("/debug_logs.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", debug_logs_html, processor); });

		// web_server.on("/web_settings_widgets.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_settings_widgets_html, processor); });

		// web_server.on("/web_settings_themes.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_settings_themes_html, processor); });

		// web_server.on("/web_settings_web.html", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", index_settings_web_html, processor); });

		web_server.onNotFound([](AsyncWebServerRequest *request) { request->send(404, "text/plain", "Not found"); });

		// web_server.on("/update_widget_ow", HTTP_POST, [](AsyncWebServerRequest *request) {
		// 	AsyncWebParameter *ow_enable = request->getParam("_set_widget_ow_enable", true);
		// 	settings.config.open_weather.enabled = (String(ow_enable->value().c_str()) == "1");

		// 	AsyncWebParameter *ow_api_key = request->getParam("_set_widget_ow_api_key", true);
		// 	settings.config.open_weather.api_key = String(ow_api_key->value().c_str());
		// 	settings.config.open_weather.api_key.trim();

		// 	AsyncWebParameter *ow_poll_frequency = request->getParam("_set_widget_ow_poll_frequency", true);
		// 	settings.config.open_weather.poll_frequency = String(ow_poll_frequency->value().c_str()).toInt();

		// 	Serial.println("Widget OW Save!");

		// 	Buzzer({{2000, 100}});
		// 	request->send(200, "text/plain", "Settings Saved!");
		// });

		web_server.on("/update_settings_group", HTTP_POST, [](AsyncWebServerRequest *request) {
			if (request->hasParam("group_id", true))
			{
				const AsyncWebParameter *_group = request->getParam("group_id", true);
				int group_id = String(_group->value().c_str()).toInt();

				Serial.printf("** Save Settings for Group ID: %d\n", group_id);

				auto &group = settings.settings_groups[group_id]; // Cache the current group

				for (size_t i = 0; i < group.groups.size(); ++i)
				{
					auto *setting = group.groups[i]; // Cache the current setting

					String fn = setting->fieldname;
					fn.replace(" ", "_");
					fn.toLowerCase();
					fn.replace("_(sec)", "");
					fn.replace("_(min)", "");
					fn.replace("_(%%)", "");

					if (setting->getType() == SettingsOptionBase::INT_VECTOR)
					{
						SettingsOptionIntVector *intPtr = static_cast<SettingsOptionIntVector *>(setting);
						for (size_t v = 0; v < intPtr->vector_size(); v++)
						{
							String fn_indexed = String(group_id) + "," + String(i) + "__" + fn + "_" + String(v);

							// Serial.printf("Looking for id: %s - ", fn_indexed.c_str());

							if (request->hasParam(fn_indexed, true))
							{
								// Serial.print("Found - ");
								const AsyncWebParameter *_param = request->getParam(fn_indexed, true);
								int data = String(_param->value().c_str()).toInt();

								// Serial.printf("Web data: %d, class data %d, change? %s\n", data, intPtr->get(v), (intPtr->update(v, data) ? "YES" : "no"));
								intPtr->update(v, data);
							}
						}
					}
					else if (setting->getType() == SettingsOptionBase::WIFI_STATION)
					{
						SettingsOptionWiFiStations *intPtr = static_cast<SettingsOptionWiFiStations *>(setting);
						for (size_t v = 0; v <= intPtr->vector_size(); v++)
						{
							String fn_ssid = String(group_id) + "," + String(i) + "__" + fn + "_ssid_" + String(v);
							String fn_pass = String(group_id) + "," + String(i) + "__" + fn + "_pass_" + String(v);

							// 1,6__wifi_stations_ssid_0
							if (request->hasParam(fn_ssid, true) && request->hasParam(fn_pass, true))
							{
								// Serial.print("Found - ");
								const AsyncWebParameter *_param1 = request->getParam(fn_ssid, true);
								String data1 = String(_param1->value().c_str());

								const AsyncWebParameter *_param2 = request->getParam(fn_pass, true);
								String data2 = String(_param2->value().c_str());

								data1.trim();
								data2.trim();

								if (v == intPtr->vector_size())
								{
									// this is a new one, so we add it, assuming there is data to add
									if (!data1.isEmpty() && !data2.isEmpty())
										intPtr->add_station(data1, data2);
								}
								else
								{
									// We update all stations, even with empty data, to ensure we keep the vector intact
									// after the updates, we'll prune any that are empty.
									intPtr->update(v, data1, data2);
								}
							}
						}

						// remove empty entries
						intPtr->remove_if_empty();
					}
					else
					{
						String fn_indexed = String(group_id) + "," + String(i) + "__" + fn;

						// Serial.printf("Looking for id: %s - ", fn_indexed.c_str());

						if (request->hasParam(fn_indexed, true))
						{
							const AsyncWebParameter *_param = request->getParam(fn_indexed, true);

							if (setting->getType() == SettingsOptionBase::BOOL)
							{
								bool data = (String(_param->value().c_str()) == "1");
								SettingsOptionBool *intPtr = static_cast<SettingsOptionBool *>(setting);

								// Serial.printf("Web data (new): %s, class data (current): %s - ", (data ? "T" : "F"), (intPtr->get() ? "T" : "F"));
								bool updated = intPtr->update(data);
								// Serial.printf("Now: %s - changed? %s\n", (intPtr->get() ? "T" : "F"), (updated ? "YES" : "no"));
							}
							else if (setting->getType() == SettingsOptionBase::FLOAT)
							{
								float data = String(_param->value().c_str()).toFloat();
								SettingsOptionFloat *intPtr = static_cast<SettingsOptionFloat *>(setting);
								// Serial.printf("Web data: %f, class data %f - ", data, intPtr->get());
								bool updated = intPtr->update(data);
								// Serial.printf("changed? %s\n", (updated ? "YES" : "no"));
							}
							else if (setting->getType() == SettingsOptionBase::STRING)
							{
								String data = String(_param->value().c_str());
								SettingsOptionString *intPtr = static_cast<SettingsOptionString *>(setting);
								// Serial.printf("Web data: %s, class data %s - ", data, intPtr->get());
								bool updated = intPtr->update(&data);
								// Serial.printf("changed? %s\n", (updated ? "YES" : "no"));
							}
							else if (setting->getType() == SettingsOptionBase::HEXCOLOR)
							{
								const char *data = _param->value().c_str();
								SettingsOptionColor565 *intPtr = static_cast<SettingsOptionColor565 *>(setting);
								Serial.printf("Web data: %s, class data %d, converted data %d - ", data, intPtr->get(), settings.webHexToColor565(data));
								bool updated = intPtr->update(settings.webHexToColor565(data));
								Serial.printf("changed? %s\n", (updated ? "YES" : "no"));
							}
							else if (setting->getType() == SettingsOptionBase::INT)
							{
								int data = String(_param->value().c_str()).toInt();
								SettingsOptionInt *intPtr = static_cast<SettingsOptionInt *>(setting);
								// Serial.printf("Web data: %d, class data %d - ", data, intPtr->get());
								bool updated = intPtr->update(data);
								// Serial.printf("changed? %s\n", (updated ? "YES" : "no"));
							}
							else if (setting->getType() == SettingsOptionBase::INT_RANGE)
							{
								int data = String(_param->value().c_str()).toInt();
								SettingsOptionIntRange *intPtr = static_cast<SettingsOptionIntRange *>(setting);
								// Serial.printf("Web data: %d, class data %d - ", data, intPtr->get());
								bool updated = intPtr->update(data);
								// Serial.printf("changed? %s\n", (updated ? "YES" : "no"));
							}
							else if (setting->getType() == SettingsOptionBase::FLOAT_RANGE)
							{
								float data = String(_param->value().c_str()).toFloat();
								SettingsOptionFloatRange *intPtr = static_cast<SettingsOptionFloatRange *>(setting);
								Serial.printf("Web data: %f, class data %f - ", data, intPtr->get());
								bool updated = intPtr->update(data);
								Serial.printf("changed? %s\n", (updated ? "YES" : "no"));
							}
						}
					}
				}

				// // Buzzer({{2000, 20}});
				// audio.play_dock();
				settings.save(true);

				const char *return_data = generate_settings_html(group_id).c_str();
				const size_t return_data_length = strlen_P(return_data);

				AsyncResponseStream *response = request->beginResponseStream("text/html", return_data_length);
				for (int i = 0; i < return_data_length; i++)
				{
					response->write(return_data[i]);
				}
				request->send(response);

				if (group_id == 6)
				{
					// we updated from Screenie, so let's re-take the screenshot
					// and update the ebsite via server events
					squixl.delayed_take_screenshot();
				}
			}
			else
			{
				request->send(200, "text/html", "<div class='container'><h2>ERROR POSTING DATA</h2><div>", processor);
			}
		});

		// web_server.on("/update_settings_watch", HTTP_POST, [](AsyncWebServerRequest *request) {
		// 	for (size_t g = 0; g < settings.setting_groups.size(); g++)
		// 	{
		// 		auto &group = settings.setting_groups[g]; // Cache the current group
		// 		for (size_t i = 0; i < group.size(); ++i)
		// 		{
		// 			auto *setting = group[i]; // Cache the current setting

		// 			String fn = setting->fieldname;
		// 			fn.replace(" ", "_");
		// 			fn.toLowerCase();
		// 			fn.replace("_(sec)", "");
		// 			fn.replace("_(%%)", "");

		// 			if (setting->getType() != SettingsOptionBase::INT_VECTOR)
		// 			{
		// 				fn = String(g) + "," + String(i) + "__" + fn;

		// 				Serial.printf("Looking for id: %s - ", fn.c_str());

		// 				if (request->hasParam(fn, true))
		// 				{
		// 					Serial.print("Found - ");
		// 					AsyncWebParameter *_param = request->getParam(fn, true);
		// 					Serial.printf("Data: %s\n", String(_param->value().c_str()));
		// 					// 	settings.config.look_ahead = String(_set_reflow_lookahead->value().c_str()).toInt();
		// 				}
		// 				else
		// 				{
		// 					Serial.println("...");
		// 				}
		// 			}
		// 			else
		// 			{
		// 				SettingsOptionIntVector *intPtr = static_cast<SettingsOptionIntVector *>(setting);
		// 				for (size_t v = 0; v < intPtr->vector_size(); v++)
		// 				{
		// 					String fn_indexed = String(g) + "," + String(i) + "__" + fn + "_" + String(v);

		// 					Serial.printf("Looking for id: %s - ", fn_indexed.c_str());

		// 					if (request->hasParam(fn_indexed, true))
		// 					{
		// 						Serial.print("Found - ");
		// 						AsyncWebParameter *_param = request->getParam(fn_indexed, true);
		// 						Serial.printf("Data: %s\n", String(_param->value().c_str()));
		// 						// 	settings.config.look_ahead = String(_set_reflow_lookahead->value().c_str()).toInt();
		// 					}
		// 					else
		// 					{
		// 						Serial.println("...");
		// 					}
		// 				}
		// 			}
		// 		}
		// 	}

		// 	Buzzer({{2000, 20}});
		// 	request->send(200, "text/plain", "Settings Saved!");
		// });

		Serial.println("web_server.begin();");
		web_server.addHandler(&web_event);
		web_server.begin();
		_running = true;
	}
	else
	{
		_running = false;
		Serial.println("Failed to connect to wifi to start webserver!");
		setCpuFrequencyMhz(40);
	}
}

void WebServer::stop(bool restart)
{
	Serial.println("Webserver stop");
	web_server.end();
	wifi_controller.wifi_prevent_disconnect = false;
	wifi_controller.disconnect(true);
	_running = false;
}

void WebServer::do_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
	// String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
	// Serial.println(logmessage);

	filename = "user_wallpaper.jpg";

	String logmessage = "";

	if (!index)
	{
		logmessage = "Upload Start: " + String(filename);
		// open the file on first call and store the file handle in the request object
		request->_tempFile = LittleFS.open("/" + filename, "w");
		Serial.println(logmessage);
	}

	if (len)
	{
		// stream the incoming chunk to the opened file
		request->_tempFile.write(data, len);
		// logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
		Serial.println(logmessage);
	}

	if (final)
	{
		logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
		// close the file handle as the upload is now done
		request->_tempFile.close();
		Serial.println(logmessage);
		squixl.main_screen()->show_user_background_jpg(false);
		request->redirect("/");
	}
}

void WebServer::process() {}

bool WebServer::is_running() { return _running; }

WebServer webserver;