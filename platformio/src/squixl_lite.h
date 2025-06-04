/*
	Base library for SQUiXL
	Copyright (c) 2025 Unexpected Maker
	https://squixl.io

	version 0.1
*/

#ifndef SQUIXL_LITE_H
#define SQUIXL_LITE_H

#include <vector>
#include <string>
#include <map>

#include "Arduino.h"
#include "Wire.h"

// Peripherals
#include "UM_LCA9555.h"
#include "xgt911.h"
#include <bb_spi_lcd.h>

// Audio
#if __has_include("audio/audio.h")
#include "audio/audio.h"
#define AUDIO_AVAILABLE
#elif __has_include("audio.h")
#include "audio.h"
#define AUDIO_AVAILABLE
#else
#warning "Audio.h not found—audio support disabled"
#endif

// SD Card Stuff
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// UI stuff
#include "fonts/ubuntu_mono_all_r.h"
#include "fonts/ubuntu_mono_all_b.h"

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

#define LCD_DELAY 0xff

constexpr uint16_t RGB(uint8_t r, uint8_t g, uint8_t b)
{
	//   return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
	return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
}

constexpr float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

constexpr uint16_t darken565(uint16_t color, float amount)
{
	if (amount <= 0.0f)
		return color;
	if (amount >= 1.0f)
		return 0;

	float scale = 1.0f - amount;

	// Extract RGB565 components
	uint8_t r5 = (color >> 11) & 0x1F;
	uint8_t g6 = (color >> 5) & 0x3F;
	uint8_t b5 = color & 0x1F;

	// Apply darkening directly to raw RGB565 components, keeping it simple
	r5 = roundf(r5 * scale);
	g6 = roundf(g6 * scale);
	b5 = roundf(b5 * scale);

	// Re-pack and return
	return (r5 << 11) | (g6 << 5) | b5;
}

constexpr uint16_t lighten565(uint16_t color, float amount)
{
	// amount = 0.0 → original, 1.0 → white (0xFFFF)
	if (amount <= 0.0f)
		return color;
	if (amount >= 1.0f)
		return 0xFFFF;

	// Extract RGB565 components
	uint8_t r5 = (color >> 11) & 0x1F;
	uint8_t g6 = (color >> 5) & 0x3F;
	uint8_t b5 = color & 0x1F;

	// Linearly interpolate each channel toward its max
	r5 = roundf(r5 + (31 - r5) * amount);
	g6 = roundf(g6 + (63 - g6) * amount);
	b5 = roundf(b5 + (31 - b5) * amount);

	// Re-pack and return
	return (r5 << 11) | (g6 << 5) | b5;
}

const uint8_t st7701s_init_commands[] = {
	1, 0x11,
	LCD_DELAY, 120,
	// PAGE0 1
	6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x10,
	3, 0xc0, 0x3b, 0x00, // Display Line Setting
	3, 0xc1, 0x0d, 0x02, // Porch Control
						 // 3, 0xc2, 0x21, 0x08, // Inversion set
	3, 0xc2, 0x81, 0x08,
	2, 0xcd, 0x08,
	17, 0xb0, 0x00, 0x11, 0x18, 0x0e, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0f, 0xaa, 0x31, 0x18,
	17, 0xb1, 0x00, 0x11, 0x19, 0x0e, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xa9, 0x32, 0x18,
	6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x11,
	2, 0xb0, 0x60,
	2, 0xb1, 0x27,
	2, 0xb2, 0x87,
	2, 0xb3, 0x80,
	2, 0xb5, 0x49,
	2, 0xb7, 0x85,
	2, 0xb8, 0x21,
	2, 0xc1, 0x78,
	2, 0xc2, 0x78,
	LCD_DELAY, 20,
	4, 0xe0, 0x00, 0x1b, 0x02,
	12, 0xe1, 0x08, 0xa0, 0x00, 0x00, 0x07, 0xa0, 0x00, 0x00, 0x00, 0x44, 0x44,
	13, 0xe2, 0x11, 0x11, 0x44, 0x44, 0xed, 0xa0, 0x00, 0x00, 0xec, 0xa0, 0x00, 0x00,
	5, 0xe3, 0x00, 0x00, 0x11, 0x11,
	3, 0xe4, 0x44, 0x44,
	17, 0xe5, 0x0a, 0xe9, 0xd8, 0xa0, 0x0c, 0xeb, 0xd8, 0xa0, 0x0e, 0xed, 0xd8, 0xa0, 0x10, 0xef, 0xd8, 0xa0,
	5, 0xe6, 0x00, 0x00, 0x11, 0x11,
	3, 0xe7, 0x44, 0x44,
	17, 0xe8, 0x09, 0xe8, 0xd8, 0xa0, 0x0b, 0xea, 0xd8, 0xa0, 0x0d, 0xec, 0xd8, 0xa0, 0x0f, 0xee, 0xd8, 0xa0,
	8, 0xeb, 0x02, 0x00, 0xe4, 0xe4, 0x88, 0x00, 0x40,
	3, 0xec, 0x3c, 0x00,
	17, 0xed, 0xab, 0x89, 0x76, 0x54, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x20, 0x45, 0x67, 0x98, 0xba,
	6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x00,
	2, 0x36, 0x00,
	2, 0x3a, 0x66,
	1, 0x21, // sleep out
	LCD_DELAY, 180,
	LCD_DELAY, 180,
	1, 0x29, // display on
	LCD_DELAY, 120,
	0
};

// const uint8_t st7701s_init_commands_new[] = {
// 	1, 0x11,
// 	LCD_DELAY, 120,
// 	// PAGE0 1
// 	6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x10,
// 	3, 0xc0, 0x3b, 0x00, // Display Line Setting
// 	3, 0xc1, 0x0d, 0x02, // Porch Control
// 						 // 3, 0xc2, 0x37, 0x05,	   // Inversion set
// 	3, 0xc2, 0x21, 0x08, // Inversion set
// 	// 4, 0xc3, 0x02, 0x80, 0x80, // SET RGB MODE  - 00-DE MODE ,80-HV MODE	 PCLK N
// 	2, 0xcd, 0x08,

// 	17, 0xb0, 0x00, 0x11, 0x18, 0x0e, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0f, 0xaa, 0x31, 0x18, // Positive Voltage Gamma Control
// 	17, 0xb1, 0x00, 0x11, 0x19, 0x0e, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xa9, 0x32, 0x18, // Negative Voltage Gamma Control

// 	//*******power set********//
// 	// PAGE2

// 	6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x11,
// 	2, 0xb0, 0x60, // Vop Amplitude setting - Vop=4.7375v
// 	2, 0xb1, 0x26, // VCOM amplitude setting - VCOM=32
// 	2, 0xb2, 0x87, // VGH Voltage setting - VGH=13v approx
// 	2, 0xb3, 0x80, // TEST Command Setting
// 	2, 0xb5, 0x49, // VGL Voltage setting VGL=-10.17v
// 	2, 0xb7, 0x85, // Power Control 1
// 	2, 0xb8, 0x21, // Power Control 2 - AVDD=6.6 & AVCL=-4.6
// 	2, 0xc1, 0x78, // Source pre_drive timing set1
// 	2, 0xc2, 0x78, // Source EQ2 Setting
// 	LCD_DELAY, 20,
// 	//*********GIP SET*************//
// 	4, 0xe0, 0x00, 0x1b, 0x02,
// 	12, 0xe1, 0x08, 0xa0, 0x00, 0x00, 0x07, 0xa0, 0x00, 0x00, 0x00, 0x44, 0x44,
// 	13, 0xe2, 0x11, 0x11, 0x44, 0x44, 0xed, 0xa0, 0x00, 0x00, 0xec, 0xa0, 0x00, 0x00,
// 	5, 0xe3, 0x00, 0x00, 0x11, 0x11,
// 	3, 0xe4, 0x44, 0x44,
// 	17, 0xe5, 0x0a, 0xe9, 0xd8, 0xa0, 0x0c, 0xeb, 0xd8, 0xa0, 0x0e, 0xed, 0xd8, 0xa0, 0x10, 0xef, 0xd8, 0xa0,
// 	5, 0xe6, 0x00, 0x00, 0x11, 0x11,
// 	3, 0xe7, 0x44, 0x44,
// 	17, 0xe8, 0x09, 0xe8, 0xd8, 0xa0, 0x0b, 0xea, 0xd8, 0xa0, 0x0d, 0xec, 0xd8, 0xa0, 0x0f, 0xee, 0xd8, 0xa0,
// 	8, 0xeb, 0x02, 0x00, 0xe4, 0xe4, 0x88, 0x00, 0x40,
// 	3, 0xec, 0x3c, 0x00,
// 	17, 0xed, 0xab, 0x89, 0x76, 0x54, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x20, 0x45, 0x67, 0x98, 0xba,
// 	//-----------VAP & VAN---------------//
// 	// 6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x13,
// 	// 2, 0xE5, 0xE4,
// 	6, 0xff, 0x77, 0x01, 0x00, 0x00, 0x00,
// 	2, 0x36, 0x00, // 10-180
// 	2, 0x3A, 0x66, // 18 bit
// 	1, 0x21,	   // turn invert on
// 	LCD_DELAY, 120,
// 	LCD_DELAY, 120,
// 	1, 0x29, // display on
// 	LCD_DELAY, 120
// };

// Here for backup purposes
const BB_RGB rgbpanel_UM_480x480 = {
	-1 /* CS */, -1 /* SCK */, -1 /* SDA */,
	38 /* DE */, 47 /* VSYNC */, 48 /* HSYNC */, 39 /* PCLK */,
	8 /* R0 */, 7 /* R1 */, 6 /* R2 */, 5 /* R3 */, 4 /* R4 */,
	14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
	21 /* B0 */, 18 /* B1 */, 17 /* B2 */, 16 /* B3 */, 15 /* B4 */,
	10 /* hsync_back_porch */, 50 /* hsync_front_porch */, 8 /* hsync_pulse_width */,
	8 /* vsync_back_porch */, 8 /* vsync_front_porch */, 3 /* vsync_pulse_width */,
	1 /* hsync_polarity */, 1 /* vsync_polarity */,
	480, 480,
	12000000 // speed
};

enum MUX_STATE
{
	MUX_OFF = 0,
	MUX_I2S = 1,
	MUX_SD = 2,
};

enum FONT_SPEC
{
	FONT_WEIGHT_R = 0,
	FONT_WEIGHT_B = 1,
};

struct font_size_t
{
		FONT_SPEC weight;
		uint8_t size;
		uint8_t width;
		uint8_t height;

		void print()
		{
			Serial.printf("font: weight %d, size %d with (%d, %d)\n", weight, size, width, height);
		}

		bool check_fit(uint8_t _size, FONT_SPEC _weight)
		{
			return (size == _size && weight == _weight);
		}
};

class SQUiXL_LITE
{
	public:
		// Base screen reference
		BB_SPI_LCD lcd;

		// Screen Functions
		void init();
		void screen_init_spi_bitbanged(const uint8_t *data);

		bool process_touch_lite(uint16_t *x, uint16_t *y);

		// IOMUX I2S & SD functions
		bool mux_switch_to(MUX_STATE new_state);
		void mux_toggle();
		bool mux_check_state(MUX_STATE new_state);

		bool vbus_present();

		bool char_sizes_cached = false;
		void cache_text_sizes();
		void get_cached_char_sizes(FONT_SPEC weight, uint8_t size, uint8_t *width, uint8_t *height);

		void show_error(String error, bool fade = false);

		// std::vector<font_size_t> font_char_sizes;

	protected:
		bool _mux_initialised = false;
		MUX_STATE _current_mux_state = MUX_STATE::MUX_OFF;
		unsigned long next_mux_switch = 0;

		bool is_5V_detected = true;
		bool font_sizes_cached = false;

		bool isTouched = false;
		unsigned long next_touch = 0;
		uint16_t touch_rate = 25;
};

extern SQUiXL_LITE squixl_lite;

#endif // SQUIXL_LITE_H