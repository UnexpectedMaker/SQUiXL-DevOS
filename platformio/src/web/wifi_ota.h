#pragma once
#include "Arduino.h"
#include <ArduinoOTA.h>

void start_ota();
void stop_ota();

bool is_ota_setup = false;
bool is_ota_updating = false;
