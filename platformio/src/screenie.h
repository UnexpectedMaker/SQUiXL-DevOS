#pragma once
#include <functional>

// Forward declare your display class if needed (or include the right header)
class BB_SPI_LCD;

// The callback type: pass bool for success/failure
using ScreenshotCallback = std::function<void(bool success)>;

// The function to call
bool request_screenshot(BB_SPI_LCD *screen, ScreenshotCallback cb);