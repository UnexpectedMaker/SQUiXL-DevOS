#ifndef SQUIXL_H
#define SQUIXL_H

#include "squixl_lite.h"

#include <vector>

#include <JPEGDisplay.h>
#include <PNGDisplay.h>

#include "ui/wallpaper/wallpaper_01.h"
#include "ui/wallpaper/wallpaper_02.h"
#include "ui/wallpaper/wallpaper_03.h"
#include "ui/wallpaper/wallpaper_04.h"
#include "ui/wallpaper/wallpaper_05.h"
#include "ui/wallpaper/wallpaper_06.h"

#include "ui/ui_animations.h"

#include "settings/settings.h"
#include <LittleFS.h>

// SD Card Stuff
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Peripherals
#include "peripherals/rtc.h"
#include "peripherals/battery.h"
#include "peripherals/haptics.h"

#include "web/webserver.h"
#include "web/wifi_controller.h"
#include "web/wifi_setup.h"

// EXP IO
#define BL_EN 0
#define LCD_RST 1
#define TP_RST 5
#define SOFT_PWR 6
#define MUX_SEL 8
#define MUX_EN 9
#define HAPTICS_EN 10
#define VBUS_SENSE 11
#define SD_DETECT 15

// ESP32-S3 IO
#define BL_PWM 40
#define TP_INT 3

// IO MUX
#define MUX_D1 41
#define MUX_D2 42
#define MUX_D3 45
#define MUX_D4 46

// CPU speeds
#define CPU_FREQ_LOW 40
#define CPU_FREQ_LOW_WIFI 80
#define CPU_FREQ_MAX 240

#define WAKE_REASON_TOUCH BIT64(GPIO_NUM_3)

enum Directions
{
	UP = 0,
	RIGHT = 1,
	DOWN = 2,
	LEFT = 3,
	UR = 4,
	DR = 5,
	DL = 6,
	UL = 7,
	NONE = 99
};

enum CPU_SPEED
{
	CPU_CHANGE_LOW = 0,
	CPU_CHANGE_HIGH = 1,
};

enum DRAGABLE
{
	DRAG_NONE = 0,
	DRAG_VERTICAL = 1,
	DRAG_HORIZONTAL = 2,
	DRAG_BOTH = 3,
};

typedef std::function<void(void)> _CALLBACK_DS;

class ui_element;
class ui_screen;
class ui_control_textbox;

// extern ui_screen screen_settings;

enum TouchEventType
{
	TOUCH_TAP = 0,
	TOUCH_DOUBLE = 1,
	TOUCH_LONG = 2,
	TOUCH_SWIPE_UP = 3,
	TOUCH_SWIPE_RIGHT = 4,
	TOUCH_SWIPE_DOWN = 5,
	TOUCH_SWIPE_LEFT = 6,
	TOUCH_DRAG = 7,
	TOUCH_DRAG_END = 8,
	SCREEN_DRAG_H = 9,
	SCREEN_DRAG_V = 10,
	TOUCH_UNKNOWN = 99,
};

struct touch_event_t
{
		uint8_t finger = 0;
		uint16_t x = 0;
		uint16_t y = 0;
		TouchEventType type = TOUCH_UNKNOWN;
		int16_t d_x = 0;
		int16_t d_y = 0;

	public:
		touch_event_t(uint8_t _finger, uint16_t _x, uint16_t _y) : finger(_finger), x(_x), y(_y), type(TOUCH_TAP), d_x(0), d_y(0) {}

		touch_event_t() : x(0), y(0), type(TOUCH_UNKNOWN), d_x(0), d_y(0) {}

		touch_event_t(uint16_t _x, uint16_t _y, TouchEventType _type) : x(_x), y(_y), type(_type), d_x(0), d_y(0) {}

		touch_event_t(uint16_t _x, uint16_t _y, TouchEventType _type, int16_t _d_x, int16_t _d_y) : x(_x), y(_y), type(_type), d_x(_d_x), d_y(_d_y) {}
};

bool save_png(BB_SPI_LCD *screen);

class SQUiXL : public SQUiXL_LITE
{

	public:
		BB_SPI_LCD sprite;
		PNGDisplay pd;
		JPEGDisplay jd;

		uint16_t squixl_blue = RGB(0, 133, 255);

		// void loadPNG(int start_x, int start_y, const void *image_data, int image_data_size);
		void loadPNG_into(BB_SPI_LCD *sprite, int start_x, int start_y, const void *image_data, int image_data_size);
		// void loadJPEG(int start_x, int start_y, const void *image_data, int image_data_size);
		void display_logo(bool show);
		void display_first_boot(bool show);

		bool process_touch_full();

		// RTC
		uint8_t wake_reason = 0;

		// backgrounds
		uint8_t cycle_next_wallpaper();
		void set_wallpaper_index(uint8_t index);
		void animate_backlight(float from, float to, unsigned long duration);
		void set_backlight_level(float pwm_level_percent);
		float get_backlight_level() { return current_backlight_pwm; }
		void process_backlight_dimmer();

		static void get_public_ip(bool success, const String &response);
		static void get_and_update_utc_settings(bool success, const String &response);

		bool vbus_changed();
		void change_cpu_frequency(bool increase);

		const String version_firmware = "Alpha v0.2";
		const String version_year = "2025";
		const uint16_t version_build = 2;
		uint16_t version_latest = 0;

		const String get_version() { return (version_firmware + " build " + version_build); }

		void process_version(bool success, const String &response);
		bool update_available() { return (version_latest > version_build); }

		void log_heap(const char *title)
		{
			Serial.printf("\nHeap Log: %s\nHeap Size: %u of %u\n", title, ESP.getFreeHeap(), ESP.getHeapSize());
			Serial.printf("Min Heap Size: %u, Max Alloc Heap Size: %u, ", ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
			Serial.printf("PSRAM Free: %u\n", ESP.getFreePsram());
			Serial.printf("Largest PSRAM Chunk Free %u\n\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
		}

		bool showing_settings = false;

		ui_screen *current_screen();
		void set_current_screen(ui_screen *screen);

		void toggle_settings();
		bool switching_screens = false;

		// Deep sleep stuff
		bool was_sleeping();
		int woke_by();
		void go_to_sleep();

		// Animation
		void start_animation_task();

		void delayed_take_screenshot() { hint_take_screenshot = true; }
		bool hint_take_screenshot = false;
		void take_screenshot();

		// Helpers
		void split_text_into_lines(const String &text, int max_chars_per_line, std::vector<String> &lines);

	protected:
		float current_backlight_pwm = 0.0f;
		unsigned long backlight_dimmer_timer = 0;

		uint32_t current_cpu_frequency = 240;

		// Touch stuff - to be cleaned up
		unsigned long last_finger_move = 0;
		unsigned long last_touch = 0;
		unsigned long dbl_touch[2] = {0};
		unsigned long drag_rate = 0;

		int16_t deltaX = 0;
		int16_t deltaY = 0;
		uint16_t moved_x = 0;
		uint16_t moved_y = 0;
		uint16_t startX = 0;
		uint16_t startY = 0;
		int16_t clamp_delta_low = -20;
		int16_t clamp_delta_high = 20;

		uint touchTime = 0;
		int8_t tab_group_index = -1;
		DRAGABLE drag_lock = DRAGABLE::DRAG_BOTH;
		bool last_was_click = false;
		bool last_was_long = false;
		bool prevent_long_press = false;

		ui_element *currently_selected = nullptr;
		ui_screen *_current_screen = nullptr;

		// Deep sleep stuff
		std::vector<_CALLBACK_DS> pre_ds_callbacks;
		std::vector<_CALLBACK_DS> post_ds_callbacks;

		// For intro logo
		BB_SPI_LCD logo_squixl;
		BB_SPI_LCD logo_black;
		BB_SPI_LCD by_um;
		BB_SPI_LCD by_um_black;

		// First boot
		BB_SPI_LCD wifi_manager_content;
		BB_SPI_LCD wifi_icon;
};

extern SQUiXL squixl;

#endif // SQUIXL_H