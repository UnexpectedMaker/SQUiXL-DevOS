// SQUiXL GFX Library for ST7701S 480x480 display running on ESP32-S3 with 8MB Octal PSRAM.
// https://squixl.io
//
// 2025 Seon Rozenblum, Unexpected Maker
//
// Based on BB_LCD_SPI by by Larry Bank, BitBank Software, Inc.
// https://github.com/bitbank2/BB_SPI_LCD
//

#pragma once

#include <Arduino.h>
#include <Print.h>

#include "sdkconfig.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>

#ifndef _GFXFONT_H_
#define _GFXFONT_H_
typedef struct
{
		uint16_t bitmapOffset;
		uint8_t width;
		uint8_t height;
		uint8_t xAdvance;
		int8_t xOffset;
		int8_t yOffset;
} GFXglyph;

typedef struct
{
		uint8_t *bitmap;
		GFXglyph *glyph;
		uint8_t first;
		uint8_t last;
		uint8_t yAdvance;
} GFXfont;
#endif // _GFXFONT_H_

namespace umgfx
{

constexpr int DRAW_NONE = 0;
constexpr int DRAW_WITH_DMA = 4;

void setDecoderUsePSRAM(bool enable);

class UM_GFX_Canvas : public Print
{
	public:
		UM_GFX_Canvas();

		bool create(int width, int height, int fill_color = -1);
		bool createWithBuffer(int width, int height, void *buffer);
		void release();

		void force_cache_write();

		void fillScreen(int color);
		void drawPixel(int16_t x, int16_t y, uint16_t color);
		void fillRect(int x, int y, int w, int h, int color);
		void drawRect(int x, int y, int w, int h, uint16_t color);
		void fillRoundRect(int x, int y, int w, int h, int r, uint16_t color);
		void drawRoundRect(int x, int y, int w, int h, int r, uint16_t color);
		void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
		void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
		void fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color);
		void drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color);
		uint16_t color565(uint8_t r, uint8_t g, uint8_t b) const;
		uint16_t readPixel(int16_t x, int16_t y) const;
		void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
		void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
		void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color);
		void readImage(int x, int y, int w, int h, uint16_t *pixels) const;
		void pushImage(int x, int y, int w, int h, const uint16_t *pixels);
		void setAddrWindow(int x, int y, int w, int h);
		void pushPixels(const uint16_t *pixels, int count);
		int captureArea(int dst_x, int dst_y, int src_x, int src_y, int src_w, int src_h, uint16_t *pixels, bool swap565 = true);
		int merge(const uint16_t *src, uint16_t transparent, bool swap565 = false);

		int drawSprite(int x, int y, const UM_GFX_Canvas *sprite, float scale = 1.0f, int transparent = -1);

		static void blendSprite(const UM_GFX_Canvas *fg, const UM_GFX_Canvas *bg, UM_GFX_Canvas *dest, uint8_t alpha, int transparent = -1);
		static void blendSpriteColor(uint16_t color, const UM_GFX_Canvas *bg, UM_GFX_Canvas *dest, uint8_t alpha, int transparent = -1);
		static void blendSpriteAt(const UM_GFX_Canvas *fg, UM_GFX_Canvas *dest, int16_t pos_x, int16_t pos_y, uint8_t alpha, int transparent = -1);
		void maskedTint(const UM_GFX_Canvas *src, const UM_GFX_Canvas *mask, int x, int y, uint16_t tint, uint8_t alpha);
		void byteSwap(const uint16_t *src, uint16_t *dest, int pixel_count) const;
		void blurGaussian();

		uint16_t *data() { return _pixels; }
		const uint16_t *data() const { return _pixels; }
		void *getBuffer() const { return _pixels; }
		int width() const { return _width; }
		int height() const { return _height; }

		void setFreeFont(const GFXfont *font);
		void setCursor(int16_t x, int16_t y);
		int16_t getCursorX() const { return _cursor_x; }
		int16_t getCursorY() const { return _cursor_y; }
		void setTextColor(int fg, int bg = -2);
		void setWordwrap(bool wrap) { _wrap = wrap; }
		void setRotation(uint8_t rotation);
		uint8_t getRotation() const { return _rotation; }
		void setScroll(bool enable);
		void setScrollPosition(int lines);
		void setAntialias(bool enable) { _antialias = enable; }
		void setPrintFlags(int flags) { _print_flags = flags; }
		int printFlags() const { return _print_flags; }
		void setFont(int iFont);
		int fontHeight() const;

		void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w1, uint16_t *h1) const;
		virtual size_t write(uint8_t) override;
		virtual size_t write(const uint8_t *buffer, size_t size) override;

	protected:
		bool _is_dma_target = false;
		size_t _size_bytes;
		uint16_t *_pixels;

	private:
		void drawGlyph(const GFXfont &font, const GFXglyph &glyph, int16_t x, int16_t y);
		void drawFastHLine(int x, int y, int w, uint16_t color);
		void drawFastVLine(int x, int y, int h, uint16_t color);
		void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
		void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);

		const GFXfont *_current_font;
		bool _owns_buffer;
		int _width;
		int _height;
		int16_t _cursor_x;
		int16_t _cursor_y;
		int _fg_color;
		int _bg_color;
		bool _wrap = true;
		uint8_t _rotation = 0;
		bool _scroll_enabled = false;
		int _scroll_offset = 0;
		bool _antialias = false;
		int _print_flags = 0;
		bool _window_active = false;
		int _window_x = 0;
		int _window_y = 0;
		int _window_w = 0;
		int _window_h = 0;
		int _window_cursor = 0;
};

struct PanelConfig
{
		int8_t cs, sck, mosi;
		int8_t de, vsync, hsync, pclk;
		int8_t r0, r1, r2, r3, r4;
		int8_t g0, g1, g2, g3, g4, g5;
		int8_t b0, b1, b2, b3, b4;
		int16_t hsync_back_porch, hsync_front_porch, hsync_pulse_width;
		int16_t vsync_back_porch, vsync_front_porch, vsync_pulse_width;
		int8_t hsync_polarity, vsync_polarity;
		int16_t width, height;
		uint32_t speed;
};

class UM_GFX : public UM_GFX_Canvas
{
	public:
		UM_GFX();

		bool begin(const PanelConfig &config);
		void changePixelClock(uint32_t freq_hz);
		void setBrightness(uint8_t level);
		void backlight(bool on);
		void sleep(bool enabled);
		void waitForVSync(uint32_t timeout_ms = 20);
		uint8_t brightness() const { return _brightness; }
		bool backlightEnabled() const { return _backlight_on; }
		bool sleeping() const { return _sleeping; }

		uint16_t *framebuffer() const { return _framebuffer; }
		int width() const { return _width; }
		int height() const { return _height; }

	private:
		static bool IRAM_ATTR handle_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);

		PanelConfig _panel_config;
		esp_lcd_panel_handle_t _panel_handle;
		uint16_t *_framebuffer;
		volatile bool _vsync_flag;
		int _width;
		int _height;
		uint8_t _brightness;
		bool _backlight_on;
		bool _sleeping;
};

extern const PanelConfig PANEL_UM_480x480;

} // namespace umgfx

constexpr int PNGDISPLAY_CENTER = -2;

class PNGDisplay
{
	public:
		PNGDisplay();

		int loadPNG(umgfx::UM_GFX_Canvas *canvas, int x, int y, const void *data, int data_size, uint32_t bgColor = 0xffffffff);
		int getPNGInfo(int *width, int *height, int *bpp, const void *data, int data_size);
		int getLastError() const { return _last_error; }

	private:
		int _last_error;
};

constexpr int JPEGDISPLAY_CENTER = -2;

class JPEGDisplay
{
	public:
		JPEGDisplay();

		int loadJPEG(umgfx::UM_GFX_Canvas *canvas, int x, int y, const void *data, int data_size, int options = 0);
		int loadJPEG_LFS(umgfx::UM_GFX_Canvas *canvas, int x, int y, const char *path, int options = 0);
		int getJPEGInfo(int *width, int *height, int *bpp, const void *data, int data_size);
		int getJPEGInfo_LFS(int *width, int *height, int *bpp, const char *path);
		int getLastError() const { return _last_error; }

	private:
		int _last_error;
};
