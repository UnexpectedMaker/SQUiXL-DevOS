# SQUiXL DevOS - Project Overview

SQUiXL is an ESP32-S3 based smart display with a 480x480 RGB LCD, capacitive touch, audio output, haptics, and wireless connectivity. This document provides an overview of the codebase for developers.

---

## Project Structure

```
platformio/
├── src/
│   ├── main.cpp              # Application entry, boot sequence, UI setup
│   ├── squixl.cpp/h          # Platform core (backlight, touch, screens)
│   ├── squixl_lite.cpp/h     # Hardware init (display, peripherals)
│   │
│   ├── ui/                   # UI system
│   │   ├── controls/         # Buttons, sliders, toggles, textboxes
│   │   ├── widgets/          # Data widgets (time, weather, RSS, etc.)
│   │   ├── dashboard/        # Dashboard gauges
│   │   ├── icons/            # Icon images (battery, wifi, weather)
│   │   ├── images/           # Keyboard layouts, misc images
│   │   └── wallpaper/        # Background images
│   │
│   ├── settings/             # Persistent settings (JSON/LittleFS)
│   ├── web/                  # Web server and WiFi controller
│   ├── peripherals/          # Hardware drivers (battery, RTC, haptics)
│   ├── audio/                # Audio playback system
│   └── mqtt/                 # MQTT client
│
├── lib/                      # Custom libraries
│   ├── UM_GFX/               # Graphics library with SIMD blending
│   ├── UM_GT911/             # Touch controller driver
│   ├── UM_LCA9555/           # IO expander driver
│   ├── RV3028C7/             # RTC driver
│   └── Melopero BME280/      # Environmental sensor
│
├── data/                     # LittleFS filesystem (web assets, config)
└── docs/                     # Documentation
```

---

## Core Systems

### 1. Hardware Layers

**squixl_lite** (lowest level):
- ST7701S display controller initialization via SPI bitbang
- ESP32-S3 RGB peripheral setup for DMA-driven display
- IO expander (LCA9555) and touch controller (GT911) initialization
- I2S/SD card MUX switching

**squixl** (platform core):
- Inherits from `SQUiXL_LITE`
- Screen management and navigation
- Touch event processing and routing
- Backlight PWM control
- CPU frequency scaling (40/80/240 MHz)
- Deep sleep transitions

### 2. Graphics System (UM_GFX)

Custom graphics library optimized for ESP32-S3 with PSRAM framebuffer.

```cpp
// Create off-screen sprite
umgfx::UM_GFX_Canvas sprite;
sprite.create(200, 100);              // No fill
sprite.create(200, 100, TFT_BLACK);   // Fill with black

// Draw to LCD
squixl.lcd.drawSprite(x, y, &sprite, scale, transparent);

// Alpha blend (0-32 range)
UM_GFX_Canvas::blendSprite(&fg, &bg, &dest, alpha, transparent);
UM_GFX_Canvas::blendSpriteAt(&fg, &dest, x, y, alpha, transparent);

// Cleanup
sprite.release();
```

Key features:
- Big-endian RGB565 format (matches RGB panel)
- SIMD-accelerated alpha blending (`s3_simd_*.S` assembly)
- PNG/JPEG decoding with PSRAM allocation
- Transparency via `TFT_MAGENTA` (0xF81F) color key

### 3. UI System

See [ui_overview.md](ui_overview.md) for complete details.

**Class hierarchy:**
- `ui_element` - Base for all UI components
- `ui_screen` - Full-screen containers with navigation
- `ui_control_*` - Interactive controls (button, slider, toggle, textbox)
- `widget_*` - Data display widgets (time, weather, battery, etc.)
- `ui_dialogbox` - Modal dialogs
- `ui_keyboard` - On-screen keyboard

**Global instances:**
```cpp
squixl              // Platform core
squixl.lcd          // Display driver
dialogbox           // Modal dialog system
keyboard            // On-screen keyboard
```

---

## WiFi & Internet Communication

### WiFi Controller (`src/web/wifi_controller.cpp`)

The `wifi_controller` manages all network operations with a background task queue.

**Callback-based HTTP requests:**
```cpp
// Add request to queue with callback
wifi_controller.add_to_queue(
    "https://api.example.com/data",
    [](bool success, const String &response) {
        if (success) {
            // Parse response JSON
            json data = json::parse(response);
            // Update UI...
        }
        // IMPORTANT: Delete response when done
        delete &response;
    }
);
```

**Key points:**
- Requests are queued and processed by a FreeRTOS task
- Callbacks run on the main thread (safe to update UI)
- Always `delete &response` in your callback to free memory
- Check `wifi_controller.is_connected()` before queueing requests

**WiFi scanning:**
```cpp
wifi_controller.start_async_scan();

// Later, check results
if (!wifi_controller.is_scan_in_progress()) {
    const auto &networks = wifi_controller.scan_results();
    for (const auto &net : networks) {
        Serial.printf("%s (%d dBm)\n", net.name.c_str(), net.rssi);
    }
}
```

### Web Server (`src/web/webserver.cpp`)

ESPAsyncWebServer provides a configuration interface.

```cpp
// Global instance
webserver.start();

// Server-sent events for live updates
webserver.web_event.send("data", "event_name");
```

Features:
- Dynamic HTML generation for settings
- Wallpaper upload endpoint
- SSE (Server-Sent Events) for real-time updates
- Template processing

---

## Settings System (`src/settings/`)

Persistent configuration stored as JSON in LittleFS.

**Global instance:** `settings`

```cpp
// Access config values
settings.config.volume
settings.config.wifi_options
settings.config.backlight_time_step_battery

// Save changes
settings.save(true);  // true = save to flash

// Check WiFi credentials
if (settings.has_wifi_creds()) { ... }

// Update WiFi
settings.update_wifi_credentials(ssid, password);
```

**SettingsOption adapters** bind UI controls to settings:
```cpp
slider_volume->set_options_data(&settings.setting_volume);
// Now slider automatically reads/writes settings.config.volume
```

---

## Peripherals (`src/peripherals/`)

| Module | Description |
|--------|-------------|
| `battery.cpp` | MAX1704x fuel gauge (voltage, percentage, charging state) |
| `haptics.cpp` | DRV2605 haptic feedback driver |
| `rtc.cpp` | RV-3028-C7 real-time clock with NTP sync |
| `expansion.cpp` | BME280 environmental sensor (temp, humidity, pressure) |

**Usage:**
```cpp
battery.get_percentage();
battery.is_charging();

haptics.play(effect_id);

rtc.get_time();
rtc.sync_from_ntp();

expansion.get_temperature();
expansion.get_humidity();
```

---

## IO MUX (SD Card / Audio Switching)

SQUiXL shares 4 GPIO pins between the SD card and I2S audio via a hardware multiplexer. You must switch the MUX to the appropriate state before using either peripheral.

**MUX States:**
| State | Description |
|-------|-------------|
| `MUX_OFF` | MUX disabled (saves power) |
| `MUX_I2S` | Pins routed to I2S audio |
| `MUX_SD` | Pins routed to SD card |

**Switching the MUX:**
```cpp
// Switch to audio (required before playing sounds)
squixl.mux_switch_to(MUX_STATE::MUX_I2S);

// Switch to SD card (required before SD operations)
squixl.mux_switch_to(MUX_STATE::MUX_SD);

// Disable MUX to save power
squixl.mux_switch_to(MUX_STATE::MUX_OFF);

// Check current state
if (squixl.mux_check_state(MUX_STATE::MUX_I2S)) {
    // Audio is available
}
```

**Important notes:**
- The MUX defaults to `MUX_I2S` at boot
- Always check/switch MUX state before SD or audio operations
- Switching has a 500ms debounce to prevent rapid toggling
- When switching to SD, the SD card is automatically initialized
- When switching to I2S, the audio system is automatically initialized

---

## Audio System (`src/audio/`)

I2S audio output with tone generation and WAV sample playback from PROGMEM. **Requires MUX set to `MUX_I2S`.**

```cpp
// Play a tone (frequency Hz, duration in 10ms units)
audio.play_tone(1000, 5);  // 1kHz for 50ms

// Volume control (0-100)
audio.set_volume(80);
```

---

## MQTT (`src/mqtt/`)

MQTT client for IoT integration.

```cpp
// Configure in settings
settings.config.mqtt.server = "mqtt.example.com";
settings.config.mqtt.port = 1883;

// Subscribe to topics, handle messages via callbacks
```

---

## Memory Management

**PSRAM usage** - Large allocations must use PSRAM:
```cpp
// UI objects use overloaded new operator
ui_screen *screen = new ui_screen();  // Auto PSRAM

// Manual allocation
void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
heap_caps_free(ptr);

// Sprites allocate in PSRAM via create()
sprite.create(480, 480);
```

**Heap monitoring:**
```cpp
log_heap();  // Print heap stats to serial
```

---

## Main Loop Flow

```cpp
void loop() {
    // 1. Process touch input
    squixl.process_touch();

    // 2. WiFi controller processes queued requests/callbacks
    wifi_controller.loop();

    // 3. Web server processes HTTP requests
    webserver.process();

    // 4. MQTT processing
    mqtt.loop();

    // 5. UI refresh (screens check should_refresh())
    squixl.current_screen()->refresh();

    // 6. Background tasks (battery monitoring, etc.)
    ...
}
```

---

## Adding a New Widget

1. Create `src/ui/widgets/widget_mywidget.h` and `.cpp`
2. Inherit from `ui_element`
3. Implement `redraw(uint8_t fade_amount, int8_t tab_group)`
4. Create sprites in `redraw()` when `!is_setup`
5. Draw content to `_sprite_content`
6. Draw to parent: `get_ui_parent()->_sprite_content.drawSprite(...)`

Example skeleton:
```cpp
class widgetMyWidget : public ui_element {
public:
    void create(int16_t x, int16_t y, int16_t w, int16_t h, ...);
    bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
    bool process_touch(touch_event_t event) override;

private:
    bool is_setup = false;
};
```

---

## Adding a New Screen

```cpp
// In main.cpp setup_ui()
screen_myscreen = new ui_screen();
screen_myscreen->setup(background_color, true);

// Add widgets/controls
screen_myscreen->add_child_ui(&my_widget);

// Link navigation (swipe from main screen down goes to myscreen)
screen_main->set_navigation(Directions::DOWN, screen_myscreen, true);
```

---

## Debugging Tips

- Use `Serial.printf()` for logging
- `log_heap()` to check memory usage
- Watch for `is_busy` flags preventing re-entrant drawing
- `TFT_MAGENTA` pixels are transparent - check your masks
- WiFi callbacks must `delete &response` to prevent leaks
