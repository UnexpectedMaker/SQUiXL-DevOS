// SQUiXL GFX Library for ST7701S 480x480 display running on ESP32-S3 with 8MB Octal PSRAM.
// https://squixl.io
//
// 2025 Seon Rozenblum, Unexpected Maker
//
// Based on BB_LCD_SPI by by Larry Bank, BitBank Software, Inc.
// https://github.com/bitbank2/BB_SPI_LCD
//

#include "UM_GFX.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <vector>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_psram.h>
#include <esp_cache.h>
#include <rom/cache.h>
#include <pgmspace.h>
#include <JPEGDEC.h>
#ifdef INTELSHORT
#undef INTELSHORT
#endif
#ifdef INTELLONG
#undef INTELLONG
#endif
#ifdef MOTOSHORT
#undef MOTOSHORT
#endif
#ifdef MOTOLONG
#undef MOTOLONG
#endif
#include <PNGdec.h>
#include <LittleFS.h>

static bool g_decoder_use_psram = true;

namespace umgfx
{
void setDecoderUsePSRAM(bool enable)
{
	g_decoder_use_psram = enable;
}
} // namespace umgfx

inline void *psram_alloc(size_t bytes)
{
	uint32_t caps = MALLOC_CAP_8BIT;
	if (g_decoder_use_psram && esp_psram_get_size() > 0)
	{
		caps = MALLOC_CAP_SPIRAM;
	}
	return heap_caps_aligned_alloc(16, bytes, caps);
}

inline void psram_free(void *ptr)
{
	if (!ptr)
		return;
	heap_caps_free(ptr);
}

inline uint16_t encode565(uint16_t color)
{
	return __builtin_bswap16(color);
}

inline uint16_t decode565(uint16_t color)
{
	return __builtin_bswap16(color);
}

namespace umgfx
{

static inline void memset16(uint16_t *dest, uint16_t value, size_t count)
{
	if (!dest || count == 0)
		return;

	uint32_t packed = (static_cast<uint32_t>(value) << 16) | value;
	uintptr_t addr = reinterpret_cast<uintptr_t>(dest);

	if (addr & 0x2)
	{
		*dest++ = value;
		--count;
		addr += 2;
	}

	uint32_t *dest32 = reinterpret_cast<uint32_t *>(dest);
	size_t words = count / 2;
	for (size_t i = 0; i < words; ++i)
	{
		dest32[i] = packed;
	}

	if (count & 1)
	{
		reinterpret_cast<uint16_t *>(dest32 + words)[0] = value;
	}
}

static void getStringBox(const GFXfont *font, const char *msg, int *width, int *top, int *bottom)
{
	if (!font || !msg || !width || !top || !bottom)
		return;

	GFXfont font_copy;
	memcpy_P(&font_copy, font, sizeof(font_copy));

	int miny = 1000;
	int maxy = 0;
	int cx = 0;

	for (size_t idx = 0; msg[idx]; ++idx)
	{
		int c = static_cast<uint8_t>(msg[idx]);
		if (c < font_copy.first || c > font_copy.last)
			continue;

		c -= font_copy.first;
		GFXglyph glyph;
		memcpy_P(&glyph, &font_copy.glyph[c], sizeof(glyph));

		cx += glyph.xAdvance;
		if (glyph.yOffset < miny)
			miny = glyph.yOffset;
		int glyph_bottom = glyph.height + glyph.yOffset;
		if (glyph_bottom > maxy)
			maxy = glyph_bottom;
	}

	*width = cx;
	*top = miny;
	*bottom = maxy;
}

extern "C"
{
	void s3_alpha_blend_color_be(uint16_t fgColor, uint16_t *pBG, uint16_t *pDest, uint32_t count, uint8_t alpha, const uint16_t *masks);
	void s3_alphatrans_color_be(uint16_t fgColor, uint16_t *pBG, uint16_t *pDest, uint32_t count, uint8_t alpha, uint16_t *transparent, const uint16_t *masks);
	void s3_alphatrans_be(uint16_t *pFG, uint16_t *pBG, uint16_t *pDest, uint32_t count, uint8_t alpha, uint16_t *transparent, const uint16_t *masks);
	void s3_alpha_blend_be(uint16_t *pFG, uint16_t *pBG, uint16_t *pDest, uint32_t count, uint8_t alpha, const uint16_t *masks);
	void s3_alpha_blend_at_be(uint16_t *pFG, uint16_t *pDest, uint32_t fg_pitch, uint32_t dest_pitch, uint32_t width, uint32_t height, uint8_t alpha, const uint16_t *masks);
	void s3_masked_tint_be(uint16_t *pDest, uint16_t *pSrc, uint16_t *pMask, uint16_t tintColor, uint32_t count, uint8_t alpha, const uint16_t *pMasks);
	void s3_blur_be(uint16_t *pSrc, uint16_t *pDest, uint32_t count, uint32_t pitch, const uint32_t *pMasks);
	void s3_byteswap(uint16_t *pSrc, uint16_t *pDest, int pixelCount);
}
static const uint16_t kSIMDMasks[4] = {0x001f, 0x07e0, 0x07c0, 0xf800};
static const uint32_t kBlurMasks[2] = {0x07e0f81f, 0x01004008};

UM_GFX::UM_GFX() : _panel_handle(nullptr), _framebuffer(nullptr), _vsync_flag(false), _width(0), _height(0), _brightness(255), _backlight_on(true), _sleeping(false)
{
	memset(&_panel_config, 0, sizeof(_panel_config));
}

bool UM_GFX::begin(const PanelConfig &config)
{
	_panel_config = config;
	_width = config.width;
	_height = config.height;

	esp_lcd_rgb_panel_config_t panel_config;
	memset(&panel_config, 0, sizeof(panel_config));

	panel_config.num_fbs = 1;
	// panel_config.sram_trans_align = 256;
	panel_config.data_width = 16;
	panel_config.bits_per_pixel = 16;
	panel_config.clk_src = LCD_CLK_SRC_PLL160M;
	panel_config.disp_gpio_num = -1;
	panel_config.pclk_gpio_num = config.pclk;
	panel_config.vsync_gpio_num = config.vsync;
	panel_config.hsync_gpio_num = config.hsync;
	panel_config.de_gpio_num = config.de;

	panel_config.data_gpio_nums[0] = config.g3;
	panel_config.data_gpio_nums[1] = config.g4;
	panel_config.data_gpio_nums[2] = config.g5;
	panel_config.data_gpio_nums[3] = config.r0;
	panel_config.data_gpio_nums[4] = config.r1;
	panel_config.data_gpio_nums[5] = config.r2;
	panel_config.data_gpio_nums[6] = config.r3;
	panel_config.data_gpio_nums[7] = config.r4;
	panel_config.data_gpio_nums[8] = config.b0;
	panel_config.data_gpio_nums[9] = config.b1;
	panel_config.data_gpio_nums[10] = config.b2;
	panel_config.data_gpio_nums[11] = config.b3;
	panel_config.data_gpio_nums[12] = config.b4;
	panel_config.data_gpio_nums[13] = config.g0;
	panel_config.data_gpio_nums[14] = config.g1;
	panel_config.data_gpio_nums[15] = config.g2;

	panel_config.flags.fb_in_psram = 1;
	panel_config.timings.pclk_hz = config.speed;
	panel_config.timings.h_res = config.width;
	panel_config.timings.v_res = config.height;
	panel_config.timings.flags.hsync_idle_low = (config.hsync_polarity == 0) ? 1 : 0;
	panel_config.timings.flags.vsync_idle_low = (config.vsync_polarity == 0) ? 1 : 0;
	panel_config.timings.hsync_back_porch = config.hsync_back_porch;
	panel_config.timings.hsync_front_porch = config.hsync_front_porch;
	panel_config.timings.hsync_pulse_width = config.hsync_pulse_width;
	panel_config.timings.vsync_back_porch = config.vsync_back_porch;
	panel_config.timings.vsync_front_porch = config.vsync_front_porch;
	panel_config.timings.vsync_pulse_width = config.vsync_pulse_width;
	// panel_config.bounce_buffer_size_px = 16 * 480;
	panel_config.dma_burst_size = 64;
	if (esp_lcd_new_rgb_panel(&panel_config, &_panel_handle) != ESP_OK)
	{
		_panel_handle = nullptr;
		return false;
	}
	if (esp_lcd_panel_reset(_panel_handle) != ESP_OK)
	{
		return false;
	}
	if (esp_lcd_panel_init(_panel_handle) != ESP_OK)
	{
		return false;
	}

	esp_lcd_rgb_panel_get_frame_buffer(_panel_handle, 1, (void **)&_framebuffer);

	esp_lcd_rgb_panel_event_callbacks_t cbs = {
		.on_vsync = handle_vsync
	};
	esp_lcd_rgb_panel_register_event_callbacks(_panel_handle, &cbs, this);

	if (_framebuffer == nullptr)
		return false;
	UM_GFX_Canvas::createWithBuffer(_width, _height, _framebuffer);
	_is_dma_target = true;

	return true;
}

void UM_GFX::changePixelClock(uint32_t freq_hz)
{
	if (_panel_handle)
	{
		esp_lcd_rgb_panel_set_pclk(_panel_handle, freq_hz);
	}
}

void UM_GFX::waitForVSync(uint32_t timeout_ms)
{
	if (!_panel_handle)
		return;

	uint32_t start = millis();
	while (!_vsync_flag)
	{
		if (timeout_ms && (millis() - start) >= timeout_ms)
		{
			return;
		}
		delay(0);
	}
	_vsync_flag = false;
}

void UM_GFX::setBrightness(uint8_t level)
{
	_brightness = level;
}

void UM_GFX::backlight(bool on)
{
	_backlight_on = on;
}

void UM_GFX::sleep(bool enabled)
{
	_sleeping = enabled;
}

bool IRAM_ATTR UM_GFX::handle_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
	UM_GFX *self = static_cast<UM_GFX *>(user_ctx);
	if (self)
	{
		self->_vsync_flag = true;
	}
	return false;
}

const PanelConfig PANEL_UM_480x480 = {
	-1, -1, -1,			   // SPI interface (unused)
	38, 47, 48, 39,		   // de, vsync, hsync, pclk
	8, 7, 6, 5, 4,		   // r0-r4
	14, 13, 12, 11, 10, 9, // g0-g5
	21, 18, 17, 16, 15,	   // b0-b4
	2, 2, 1,			   // hsync timings
	8, 8, 3,			   // vsync timings
	1, 1,				   // polarities
	480, 480,
	12000000
};

UM_GFX_Canvas::UM_GFX_Canvas() : _current_font(nullptr), _pixels(nullptr), _size_bytes(0), _owns_buffer(false), _width(0), _height(0), _cursor_x(0), _cursor_y(0), _fg_color(0xFFFF), _bg_color(-2), _wrap(true), _rotation(0), _scroll_enabled(false), _scroll_offset(0), _antialias(false), _print_flags(0), _window_active(false), _window_x(0), _window_y(0), _window_w(0), _window_h(0), _window_cursor(0) {}

bool UM_GFX_Canvas::create(int width, int height, int fill_color)
{
	if (width <= 0 || height <= 0)
		return false;

	_width = width;
	_height = height;
	_size_bytes = static_cast<size_t>(_width) * static_cast<size_t>(_height) * sizeof(uint16_t);

	if (esp_psram_get_size() == 0)
	{
		return false;
	}

	_pixels = static_cast<uint16_t *>(heap_caps_aligned_alloc(16, _size_bytes, MALLOC_CAP_SPIRAM));
	if (!_pixels)
	{
		_width = _height = 0;
		_size_bytes = 0;
		return false;
	}

	if (fill_color >= 0)
	{
		fillScreen(fill_color);
	}

	_owns_buffer = true;
	_cursor_x = 0;
	_cursor_y = 0;
	return true;
}

bool UM_GFX_Canvas::createWithBuffer(int width, int height, void *buffer)
{
	if (width <= 0 || height <= 0 || !buffer)
		return false;

	_width = width;
	_height = height;
	_size_bytes = static_cast<size_t>(_width) * static_cast<size_t>(_height) * sizeof(uint16_t);
	_pixels = static_cast<uint16_t *>(buffer);
	_owns_buffer = false;
	_cursor_x = 0;
	_cursor_y = 0;
	return true;
}

void UM_GFX_Canvas::release()
{
	if (_owns_buffer && _pixels)
	{
		heap_caps_free(_pixels);
	}
	_pixels = nullptr;
	_size_bytes = 0;
	_owns_buffer = false;
	_width = 0;
	_height = 0;
}

void UM_GFX_Canvas::force_cache_write()
{
	esp_cache_msync(_pixels, _size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
}

// int UM_GFX_Canvas::createVirtual(int width, int height, void *buffer, bool)
// {
// 	return create(width, height, buffer) ? 1 : 0;
// }

// int UM_GFX_Canvas::freeVirtual()
// {
// 	release();
// 	return 1;
// }

void UM_GFX_Canvas::fillScreen(int color)
{
	if (!_pixels)
		return;
	uint16_t encoded = encode565(static_cast<uint16_t>(color));
	memset16(_pixels, encoded, static_cast<size_t>(_width) * static_cast<size_t>(_height));
}

void UM_GFX_Canvas::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if (!_pixels)
		return;
	if (x < 0 || y < 0 || x >= _width || y >= _height)
		return;
	_pixels[y * _width + x] = encode565(color);
}

uint16_t UM_GFX_Canvas::readPixel(int16_t x, int16_t y) const
{
	if (!_pixels || x < 0 || y < 0 || x >= _width || y >= _height)
		return 0;
	return decode565(_pixels[y * _width + x]);
}

void UM_GFX_Canvas::fillRect(int x, int y, int w, int h, int color)
{
	if (!_pixels || w <= 0 || h <= 0)
		return;

	int start_x = std::max(0, x);
	int start_y = std::max(0, y);
	int end_x = std::min(_width, x + w);
	int end_y = std::min(_height, y + h);

	if (start_x >= end_x || start_y >= end_y)
		return;

	uint16_t encoded = encode565(static_cast<uint16_t>(color));
	for (int row = start_y; row < end_y; ++row)
	{
		uint16_t *dst = _pixels + row * _width + start_x;
		memset16(dst, encoded, static_cast<size_t>(end_x - start_x));
	}
}

void UM_GFX_Canvas::drawRect(int x, int y, int w, int h, uint16_t color)
{
	if (w <= 0 || h <= 0)
		return;
	drawFastHLine(x, y, w, color);
	drawFastHLine(x, y + h - 1, w, color);
	drawFastVLine(x, y, h, color);
	drawFastVLine(x + w - 1, y, h, color);

	// // Flush CPU cache to memory so DMA sees updated pixels (only for LCD framebuffer)
	// if (_is_dma_target)
	// 	esp_cache_msync(_pixels, _size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
}

void UM_GFX_Canvas::drawLine(int x0, int y0, int x1, int y1, uint16_t color)
{
	int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int16_t err = dx + dy, e2;

	while (true)
	{
		drawPixel(x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = 2 * err;
		if (e2 >= dy)
		{
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			y0 += sy;
		}
	}
}

void UM_GFX_Canvas::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
	drawLine(x0, y0, x1, y1, color);
	drawLine(x1, y1, x2, y2, color);
	drawLine(x2, y2, x0, y0, color);
}

void UM_GFX_Canvas::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color)
{
	if (y0 > y1)
	{
		std::swap(y0, y1);
		std::swap(x0, x1);
	}
	if (y1 > y2)
	{
		std::swap(y1, y2);
		std::swap(x1, x2);
	}
	if (y0 > y1)
	{
		std::swap(y0, y1);
		std::swap(x0, x1);
	}

	if (y0 == y2)
	{
		int minx = std::min({x0, x1, x2});
		int maxx = std::max({x0, x1, x2});
		drawFastHLine(minx, y0, maxx - minx + 1, color);
		return;
	}

	int32_t dx01 = x1 - x0;
	int32_t dy01 = y1 - y0;
	int32_t dx02 = x2 - x0;
	int32_t dy02 = y2 - y0;
	int32_t dx12 = x2 - x1;
	int32_t dy12 = y2 - y1;

	int32_t sa = 0;
	int32_t sb = 0;

	int y, last = (y1 == y2) ? y1 : y1 - 1;
	for (y = y0; y <= last; y++)
	{
		int a = x0 + sa / dy01;
		int b = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;
		if (a > b)
			std::swap(a, b);
		drawFastHLine(a, y, b - a + 1, color);
	}

	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++)
	{
		int a = x1 + sa / dy12;
		int b = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;
		if (a > b)
			std::swap(a, b);
		drawFastHLine(a, y, b - a + 1, color);
	}
}

void UM_GFX_Canvas::readImage(int x, int y, int w, int h, uint16_t *pixels) const
{
	if (!_pixels || !pixels || w <= 0 || h <= 0)
		return;

	int start_x = std::max(0, x);
	int start_y = std::max(0, y);
	int end_x = std::min(_width, x + w);
	int end_y = std::min(_height, y + h);

	if (start_x >= end_x || start_y >= end_y)
		return;

	int copy_w = end_x - start_x;

	for (int row = start_y; row < end_y; ++row)
	{
		const uint16_t *src = _pixels + row * _width + start_x;
		memcpy(pixels, src, copy_w * sizeof(uint16_t));
		pixels += w;
	}
}

void UM_GFX_Canvas::pushImage(int x, int y, int w, int h, const uint16_t *pixels)
{
	if (!_pixels || !pixels || w <= 0 || h <= 0)
		return;

	int start_x = std::max(0, x);
	int start_y = std::max(0, y);
	int end_x = std::min(_width, x + w);
	int end_y = std::min(_height, y + h);

	if (start_x >= end_x || start_y >= end_y)
		return;

	int copy_w = end_x - start_x;

	for (int row = start_y; row < end_y; ++row)
	{
		uint16_t *dst = _pixels + row * _width + start_x;
		for (int col = 0; col < copy_w; ++col)
		{
			dst[col] = encode565(pixels[col]);
		}
		pixels += w;
	}
}

void UM_GFX_Canvas::setAddrWindow(int x, int y, int w, int h)
{
	if (!_pixels || w <= 0 || h <= 0)
	{
		_window_active = false;
		return;
	}

	int start_x = std::max(0, x);
	int start_y = std::max(0, y);
	int end_x = std::min(_width, x + w);
	int end_y = std::min(_height, y + h);

	if (start_x >= end_x || start_y >= end_y)
	{
		_window_active = false;
		return;
	}

	_window_x = start_x;
	_window_y = start_y;
	_window_w = end_x - start_x;
	_window_h = end_y - start_y;
	_window_cursor = 0;
	_window_active = true;
}

void UM_GFX_Canvas::pushPixels(const uint16_t *pixels, int count)
{
	if (!_pixels || !pixels || count <= 0 || !_window_active)
		return;

	int total = _window_w * _window_h;
	int remaining = total - _window_cursor;
	if (remaining <= 0)
	{
		_window_active = false;
		return;
	}

	if (count > remaining)
		count = remaining;

	for (int i = 0; i < count; ++i)
	{
		int idx = _window_cursor++;
		int dest_x = _window_x + (idx % _window_w);
		int dest_y = _window_y + (idx / _window_w);
		if (dest_x >= 0 && dest_x < _width && dest_y >= 0 && dest_y < _height)
		{
			_pixels[dest_y * _width + dest_x] = pixels[i];
		}
	}

	if (_window_cursor >= total)
	{
		_window_active = false;
	}
}

int UM_GFX_Canvas::captureArea(int dst_x, int dst_y, int src_x, int src_y, int src_w, int src_h, uint16_t *pixels, bool swap565)
{
	if (!_pixels || !pixels)
		return 0;

	if (dst_x >= (src_x + src_w) || src_x >= (dst_x + _width) || dst_y >= (src_y + src_h) || src_y >= (dst_y + _height))
		return 0;

	int sx = dst_x - src_x;
	int sy = dst_y - src_y;
	int dx = 0;
	int dy = 0;
	int cx = _width;
	int cy = _height;

	if (sx < 0)
	{
		dx -= sx;
		cx += sx;
		sx = 0;
	}
	if (sy < 0)
	{
		dy -= sy;
		cy += sy;
		sy = 0;
	}
	cx = std::min(cx, src_w);
	cy = std::min(cy, src_h);
	cx = std::min(cx, _width - dx);
	cy = std::min(cy, _height - dy);
	if (cx <= 0 || cy <= 0)
		return 0;

	uint16_t *dest = _pixels + dx + dy * _width;
	uint16_t *src = pixels + sx + sy * src_w;

	for (int y = 0; y < cy; ++y)
	{
		if (swap565)
		{
			byteSwap(src, dest, cx);
		}
		else
		{
			memcpy(dest, src, cx * sizeof(uint16_t));
		}
		dest += _width;
		src += src_w;
	}
	return 1;
}

int UM_GFX_Canvas::merge(const uint16_t *src, uint16_t transparent, bool swap565)
{
	if (!_pixels || !src)
		return 0;

	size_t count = static_cast<size_t>(_width) * static_cast<size_t>(_height);
	uint16_t encoded_trans = encode565(transparent);
	for (size_t i = 0; i < count; ++i)
	{
		if (_pixels[i] == encoded_trans)
		{
			uint16_t value = src[i];
			if (swap565)
				value = encode565(value);
			_pixels[i] = value;
		}
	}
	return 1;
}

int UM_GFX_Canvas::drawSprite(int x, int y, const UM_GFX_Canvas *sprite, float scale, int transparent)
{
	if (!sprite || !sprite->_pixels || !_pixels)
		return -1;
	if (scale <= 0.0f)
		return -1;

	const int src_width = sprite->_width;
	const int src_height = sprite->_height;
	int dest_width = static_cast<int>(scale * static_cast<float>(src_width));
	int dest_height = static_cast<int>(scale * static_cast<float>(src_height));
	if (dest_width < 1 || dest_height < 1)
		return 0;

	if (x >= _width || y >= _height)
		return 0;

	uint32_t frac = static_cast<uint32_t>(std::round(65536.0f / scale));
	if (frac == 0)
		frac = 1;

	int clipped_x = x;
	int clipped_y = y;
	uint32_t start_x_acc = 0;
	uint32_t start_y_acc = 0;

	if (clipped_x < 0)
	{
		start_x_acc = static_cast<uint32_t>(-clipped_x) * frac;
		dest_width += clipped_x;
		clipped_x = 0;
		if (dest_width <= 0)
			return 0;
	}
	if (clipped_y < 0)
	{
		start_y_acc = static_cast<uint32_t>(-clipped_y) * frac;
		dest_height += clipped_y;
		clipped_y = 0;
		if (dest_height <= 0)
			return 0;
	}
	if (clipped_x + dest_width > _width)
	{
		dest_width = _width - clipped_x;
		if (dest_width <= 0)
			return 0;
	}
	if (clipped_y + dest_height > _height)
	{
		dest_height = _height - clipped_y;
		if (dest_height <= 0)
			return 0;
	}

	const bool has_transparency = (transparent >= 0);
	const uint16_t trans_color = encode565(static_cast<uint16_t>(transparent));
	const bool is_unscaled = (frac == 0x10000);
	uint16_t *dest_row = _pixels + clipped_y * _width + clipped_x;
	uint32_t y_acc = start_y_acc;

	for (int dy = 0; dy < dest_height; ++dy)
	{
		const uint16_t *src_row = sprite->_pixels + ((y_acc >> 16) * src_width);
		uint32_t x_acc = start_x_acc;
		if (!has_transparency && is_unscaled)
		{
			const uint16_t *src_start = src_row + (x_acc >> 16);
			memcpy(dest_row, src_start, static_cast<size_t>(dest_width) * sizeof(uint16_t));
		}
		else
		{
			for (int dx = 0; dx < dest_width; ++dx)
			{
				const uint16_t pixel = src_row[x_acc >> 16];
				if (!has_transparency || pixel != trans_color)
				{
					dest_row[dx] = pixel;
				}
				x_acc += frac;
			}
		}
		y_acc += frac;
		dest_row += _width;
	}

	// Flush CPU cache to memory so DMA sees updated pixels (only for LCD framebuffer)
	if (_is_dma_target)
		esp_cache_msync(_pixels, _size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

	return 0;
}

void UM_GFX_Canvas::setFreeFont(const GFXfont *font)
{
	_current_font = font;
}

void UM_GFX_Canvas::setCursor(int16_t x, int16_t y)
{
	_cursor_x = x;
	_cursor_y = y;
}

void UM_GFX_Canvas::setTextColor(int fg, int bg)
{
	_fg_color = fg;
	_bg_color = bg;
}

void UM_GFX_Canvas::setFont(int)
{
	// Legacy API placeholder; built-in fonts not supported.
}

void UM_GFX_Canvas::setRotation(uint8_t rotation)
{
	_rotation = rotation & 3;
}

void UM_GFX_Canvas::setScroll(bool enable)
{
	_scroll_enabled = enable;
	if (!enable)
	{
		_scroll_offset = 0;
	}
}

void UM_GFX_Canvas::setScrollPosition(int lines)
{
	if (_scroll_enabled)
	{
		_scroll_offset = lines;
	}
}

int UM_GFX_Canvas::fontHeight() const
{
	if (_current_font == nullptr)
		return 0;
	GFXfont font;
	memcpy_P(&font, _current_font, sizeof(font));
	return font.yAdvance;
}

void UM_GFX_Canvas::getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w1, uint16_t *h1) const
{
	if (!x1 || !y1 || !w1 || !h1 || !string)
		return;

	if (_current_font == nullptr)
	{
		*x1 = x;
		*y1 = y;
		*w1 = 0;
		*h1 = 0;
		return;
	}

	int w, top, bottom;
	getStringBox(_current_font, string, &w, &top, &bottom);
	*x1 = x;
	*y1 = y + top;
	*w1 = w;
	*h1 = bottom - top;
}

void UM_GFX_Canvas::drawGlyph(const GFXfont &font, const GFXglyph &glyph, int16_t x, int16_t y)
{
	if (glyph.width == 0 || glyph.height == 0)
		return;
	if (!_pixels)
		return;

	if (_bg_color >= 0)
	{
		fillRect(x, y, glyph.width, glyph.height, _bg_color);
	}

	uint32_t bo = glyph.bitmapOffset;
	uint8_t bits = 0;
	uint8_t bit = 0;
	const int base_x = x;
	const int base_y = y;
	const uint16_t encoded_fg = encode565(static_cast<uint16_t>(_fg_color));

	for (int yy = 0; yy < glyph.height; ++yy)
	{
		const int dest_y = base_y + yy;
		const bool row_visible = (dest_y >= 0 && dest_y < _height);
		int run_start = -1;
		auto flush_run = [&](int run_end) {
			if (run_start < 0 || !row_visible)
			{
				run_start = -1;
				return;
			}
			int dest_x0 = base_x + run_start;
			int dest_x1 = base_x + run_end;
			if (dest_x1 <= 0 || dest_x0 >= _width)
			{
				run_start = -1;
				return;
			}
			dest_x0 = std::max(0, dest_x0);
			dest_x1 = std::min(_width, dest_x1);
			int len = dest_x1 - dest_x0;
			if (len > 0)
			{
				uint16_t *dst = _pixels + dest_y * _width + dest_x0;
				memset16(dst, encoded_fg, static_cast<size_t>(len));
			}
			run_start = -1;
		};

		for (int xx = 0; xx < glyph.width; ++xx)
		{
			if (!(bit++ & 7))
			{
				bits = pgm_read_byte(&font.bitmap[bo++]);
			}
			if (bits & 0x80)
			{
				if (run_start < 0)
				{
					run_start = xx;
				}
			}
			else
			{
				flush_run(xx);
			}
			bits <<= 1;
		}
		flush_run(glyph.width);
	}
}

size_t UM_GFX_Canvas::write(const uint8_t *buffer, size_t size)
{
	size_t n = 0;
	while (size--)
	{
		if (write(*buffer++))
			n++;
		else
			break;
	}
	return n;
}

size_t UM_GFX_Canvas::write(uint8_t c)
{
	if (_current_font == nullptr)
		return 1;

	GFXfont font;
	memcpy_P(&font, _current_font, sizeof(font));

	uint8_t first = font.first;
	uint8_t last = font.last;
	int16_t yAdvance = font.yAdvance;

	if (c == '\n')
	{
		_cursor_x = 0;
		_cursor_y += yAdvance;
		return 1;
	}
	else if (c == '\r')
	{
		return 1;
	}

	if (c < first || c > last)
		return 1;

	GFXglyph glyph;
	memcpy_P(&glyph, &font.glyph[c - first], sizeof(GFXglyph));

	int16_t gx = _cursor_x + glyph.xOffset;
	int16_t gy = _cursor_y + glyph.yOffset;

	if (_wrap && (gx + glyph.width) > _width)
	{
		_cursor_x = 0;
		_cursor_y += yAdvance;
		gx = _cursor_x + glyph.xOffset;
		gy = _cursor_y + glyph.yOffset;
	}

	drawGlyph(font, glyph, gx, gy);

	_cursor_x += glyph.xAdvance;
	return 1;
}

static inline uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t alpha32)
{
	const uint32_t mask = 0x07E0F81F;
	uint32_t fg32 = fg | (fg << 16);
	uint32_t bg32 = bg | (bg << 16);
	fg32 &= mask;
	bg32 &= mask;
	uint32_t bgAlpha = 32 - alpha32;
	uint32_t out = (fg32 * alpha32) + (bg32 * bgAlpha);
	out = (out >> 5) & mask;
	out |= (out >> 16);
	return static_cast<uint16_t>(out);
}

void UM_GFX_Canvas::blendSprite(const UM_GFX_Canvas *fg, const UM_GFX_Canvas *bg, UM_GFX_Canvas *dest, uint8_t alpha, int transparent)
{
	if (!fg || !bg || !dest || !fg->_pixels || !bg->_pixels || !dest->_pixels)
		return;

	int width = std::min({fg->_width, bg->_width, dest->_width});
	int height = std::min({fg->_height, bg->_height, dest->_height});
	if (width <= 0 || height <= 0)
		return;

	uint8_t alpha32 = std::min<uint8_t>(alpha, 32);
	uint16_t transparent_color = static_cast<uint16_t>(transparent);
	bool has_transparency = (transparent >= 0);

	if (!has_transparency)
	{
		s3_alpha_blend_be(const_cast<uint16_t *>(fg->_pixels), const_cast<uint16_t *>(bg->_pixels), dest->_pixels, width * height, alpha32, kSIMDMasks);
		Cache_WriteBack_Addr((uint32_t)dest->_pixels, width * height * 2);
		// if (dest->_is_dma_target)
		// 	esp_cache_msync(dest->_pixels, dest->_size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
	}
	else
	{
		uint16_t trans = transparent_color;
		s3_alphatrans_be(const_cast<uint16_t *>(fg->_pixels), const_cast<uint16_t *>(bg->_pixels), dest->_pixels, width * height, alpha32, &trans, kSIMDMasks);
		Cache_WriteBack_Addr((uint32_t)dest->_pixels, width * height * 2);
		// if (dest->_is_dma_target)
		// 	esp_cache_msync(dest->_pixels, dest->_size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
	}
}

void UM_GFX_Canvas::blendSpriteColor(uint16_t color, const UM_GFX_Canvas *bg, UM_GFX_Canvas *dest, uint8_t alpha, int transparent)
{
	if (!bg || !dest || !bg->_pixels || !dest->_pixels)
		return;

	int width = std::min(bg->_width, dest->_width);
	int height = std::min(bg->_height, dest->_height);
	if (width <= 0 || height <= 0)
		return;

	uint8_t alpha32 = std::min<uint8_t>(alpha, 32);
	uint16_t transparent_color = static_cast<uint16_t>(transparent);
	bool has_transparency = (transparent >= 0);

	if (!has_transparency)
	{
		s3_alpha_blend_color_be(color, const_cast<uint16_t *>(bg->_pixels), dest->_pixels, width * height, alpha32, kSIMDMasks);
		Cache_WriteBack_Addr((uint32_t)dest->_pixels, width * height * 2);
		// if (dest->_is_dma_target)
		// 	esp_cache_msync(dest->_pixels, dest->_size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
		return;
	}
	else
	{
		uint16_t trans = transparent_color;
		s3_alphatrans_color_be(color, const_cast<uint16_t *>(bg->_pixels), dest->_pixels, width * height, alpha32, &trans, kSIMDMasks);
		Cache_WriteBack_Addr((uint32_t)dest->_pixels, width * height * 2);
		// if (dest->_is_dma_target)
		// 	esp_cache_msync(dest->_pixels, dest->_size_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
		return;
	}
}

void UM_GFX_Canvas::blendSpriteAt(const UM_GFX_Canvas *fg, UM_GFX_Canvas *dest, int16_t pos_x, int16_t pos_y, uint8_t alpha, int transparent)
{
	if (!fg || !dest || !fg->_pixels || !dest->_pixels)
		return;

	// Calculate clipped region
	int src_x = 0, src_y = 0;
	int dst_x = pos_x, dst_y = pos_y;
	int copy_w = fg->_width;
	int copy_h = fg->_height;

	// Clip left edge
	if (dst_x < 0)
	{
		src_x = -dst_x;
		copy_w += dst_x;
		dst_x = 0;
	}
	// Clip top edge
	if (dst_y < 0)
	{
		src_y = -dst_y;
		copy_h += dst_y;
		dst_y = 0;
	}
	// Clip right edge
	if (dst_x + copy_w > dest->_width)
	{
		copy_w = dest->_width - dst_x;
	}
	// Clip bottom edge
	if (dst_y + copy_h > dest->_height)
	{
		copy_h = dest->_height - dst_y;
	}

	if (copy_w <= 0 || copy_h <= 0)
		return;

	// Round width down to multiple of 8 for SIMD
	copy_w = copy_w & ~7;
	if (copy_w <= 0)
		return;

	uint8_t alpha32 = std::min<uint8_t>(alpha, 32);

	// Calculate starting pointers
	uint16_t *fg_ptr = const_cast<uint16_t *>(fg->_pixels) + src_y * fg->_width + src_x;
	uint16_t *dest_ptr = dest->_pixels + dst_y * dest->_width + dst_x;

	// Single SIMD call handles entire sprite
	s3_alpha_blend_at_be(fg_ptr, dest_ptr, fg->_width, dest->_width, copy_w, copy_h, alpha32, kSIMDMasks);

	// Cache flush for DMA
	if (dest->_is_dma_target)
	{
		Cache_WriteBack_Addr((uint32_t)dest->_pixels, dest->_size_bytes);
	}
}

void UM_GFX_Canvas::maskedTint(const UM_GFX_Canvas *src, const UM_GFX_Canvas *mask, int x, int y, uint16_t tint, uint8_t alpha)
{
	if (!_pixels || !src || !mask || !src->_pixels || !mask->_pixels)
		return;

	int dest_x0 = std::max(0, x);
	int dest_y0 = std::max(0, y);
	int src_x0 = dest_x0 - x;
	int src_y0 = dest_y0 - y;

	int dest_x1 = std::min(_width, x + mask->_width);
	int dest_y1 = std::min(_height, y + mask->_height);

	int copy_w = dest_x1 - dest_x0;
	int copy_h = dest_y1 - dest_y0;
	if (copy_w <= 0 || copy_h <= 0)
		return;

	copy_w = std::min(copy_w, src->_width - src_x0);
	copy_h = std::min(copy_h, src->_height - src_y0);
	copy_w = std::min(copy_w, mask->_width - src_x0);
	copy_h = std::min(copy_h, mask->_height - src_y0);
	if (copy_w <= 0 || copy_h <= 0)
		return;

	bool full_cover = (dest_x0 == 0 && dest_y0 == 0 && copy_w == _width && copy_h == _height && src->_width == _width && src->_height == _height && mask->_width == _width && mask->_height == _height);
	if (full_cover)
	{
		s3_masked_tint_be(_pixels, const_cast<uint16_t *>(src->_pixels), const_cast<uint16_t *>(mask->_pixels), tint, static_cast<uint32_t>(copy_w) * static_cast<uint32_t>(copy_h), alpha, kSIMDMasks);
		Cache_WriteBack_Addr((uint32_t)_pixels, copy_w * copy_h * 2);
		return;
	}

	uint16_t *dest_row = _pixels + dest_y0 * _width + dest_x0;
	const uint16_t *src_row = src->_pixels + src_y0 * src->_width + src_x0;
	const uint16_t *mask_row = mask->_pixels + src_y0 * mask->_width + src_x0;

	uint32_t tint32 = tint | (static_cast<uint32_t>(tint) << 16);
	tint32 &= kBlurMasks[0];
	uint8_t alpha32 = std::min<uint8_t>(alpha, 32);
	uint8_t bg_alpha = 32 - alpha32;

	for (int row = 0; row < copy_h; ++row)
	{
		for (int col = 0; col < copy_w; ++col)
		{
			if (mask_row[col])
			{
				uint32_t src_val = __builtin_bswap16(src_row[col]);
				src_val |= (src_val << 16);
				src_val &= kBlurMasks[0];
				uint32_t blended = (src_val * bg_alpha) + (tint32 * alpha32);
				blended = (blended >> 5) & kBlurMasks[0];
				blended |= (blended >> 16);
				dest_row[col] = __builtin_bswap16(static_cast<uint16_t>(blended));
			}
			else
			{
				dest_row[col] = src_row[col];
			}
		}
		dest_row += _width;
		src_row += src->_width;
		mask_row += mask->_width;
	}
}

void UM_GFX_Canvas::byteSwap(const uint16_t *src, uint16_t *dest, int pixel_count) const
{
	if (!src || !dest || pixel_count <= 0)
		return;

	int leading_bytes = reinterpret_cast<uintptr_t>(src) & 0x0f;
	if (leading_bytes)
	{
		int leading_pixels = std::min(pixel_count, leading_bytes / 2);
		for (int i = 0; i < leading_pixels; ++i)
		{
			dest[i] = __builtin_bswap16(src[i]);
		}
		src += leading_pixels;
		dest += leading_pixels;
		pixel_count -= leading_pixels;
	}

	if (pixel_count >= 16 && (reinterpret_cast<uintptr_t>(src) & 0x0f) == (reinterpret_cast<uintptr_t>(dest) & 0x0f))
	{
		int chunk = pixel_count & ~0x0f;
		if (chunk > 0)
		{
			s3_byteswap(const_cast<uint16_t *>(src), dest, chunk);
			src += chunk;
			dest += chunk;
			pixel_count -= chunk;
		}
	}

	for (int i = 0; i < pixel_count; ++i)
	{
		dest[i] = __builtin_bswap16(src[i]);
	}
}

void UM_GFX_Canvas::blurGaussian()
{
	if (!_pixels || _width < 3 || _height < 3)
		return;

	const int w = _width;
	const int h = _height;
	std::vector<uint16_t> scratch(static_cast<size_t>(w) * 2);
	int buffer_index = 0;

	auto expand = [](uint16_t value) -> uint32_t {
		uint32_t v = __builtin_bswap16(value);
		v |= (v << 16);
		return v & kBlurMasks[0];
	};

	for (int y = 1; y < h - 1; ++y)
	{
		uint16_t *dest_row = scratch.data() + buffer_index * w;
		const uint16_t *src = _pixels + y * w;
		dest_row[0] = src[-1];
		dest_row[w - 1] = src[w];

		if ((w & 3) == 0)
		{
			s3_blur_be(const_cast<uint16_t *>(src), dest_row + 1, w, w * 2, kBlurMasks);
		}
		else
		{
			const uint16_t *s = src;
			for (int x = 1; x < w - 1; ++x)
			{
				uint32_t sum = expand(s[-w]);
				sum += expand(s[-w + 1]) * 2;
				sum += expand(s[-w + 2]);

				uint32_t mid = expand(s[0]);
				sum += mid * 2;

				mid = expand(s[1]);
				sum += mid * 4;

				mid = expand(s[2]);
				sum += mid * 2;

				sum += expand(s[w]);
				sum += expand(s[w + 1]) * 2;
				sum += expand(s[w + 2]);

				sum = (sum + kBlurMasks[1]) >> 4;
				sum &= kBlurMasks[0];
				sum |= (sum >> 16);
				dest_row[x] = __builtin_bswap16(static_cast<uint16_t>(sum));
				++s;
			}
		}

		buffer_index ^= 1;
		if (y > 1)
		{
			uint16_t *prev = scratch.data() + buffer_index * w;
			memcpy(_pixels + (y - 1) * w, prev, w * sizeof(uint16_t));
		}
	}

	uint16_t *final_row = scratch.data() + (buffer_index ^ 1) * w;
	memcpy(_pixels + (h - 2) * w, final_row, w * sizeof(uint16_t));
}

void UM_GFX_Canvas::drawFastHLine(int x, int y, int w, uint16_t color)
{
	if (!_pixels || y < 0 || y >= _height || w <= 0)
		return;
	int start_x = std::max(0, x);
	int end_x = std::min(_width, x + w);
	if (start_x >= end_x)
		return;
	uint16_t *dst = _pixels + y * _width + start_x;
	memset16(dst, encode565(color), static_cast<size_t>(end_x - start_x));
}

void UM_GFX_Canvas::drawFastVLine(int x, int y, int h, uint16_t color)
{
	if (!_pixels || x < 0 || x >= _width || h <= 0)
		return;
	int start_y = std::max(0, y);
	int end_y = std::min(_height, y + h);
	if (start_y >= end_y)
		return;
	uint16_t encoded = encode565(color);
	for (int row = start_y; row < end_y; ++row)
	{
		_pixels[row * _width + x] = encoded;
	}
}

void UM_GFX_Canvas::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	if (r <= 0)
		return;
	drawFastVLine(x0, y0 - r, 2 * r + 1, color);
	fillCircleHelper(x0, y0, r, 3, 0, color);
}

void UM_GFX_Canvas::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	if (r <= 0)
		return;
	drawCircleHelper(x0, y0, r, 0xF, color);
}

uint16_t UM_GFX_Canvas::color565(uint8_t r, uint8_t g, uint8_t b) const
{
	uint16_t u16 = (b >> 3);
	u16 |= ((g >> 2) << 5);
	u16 |= ((r >> 3) << 11);
	return u16;
}

void UM_GFX_Canvas::fillEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color)
{
	if (rx <= 0 || ry <= 0)
		return;
	int64_t rx_sq = (int64_t)rx * rx;
	int64_t ry_sq = (int64_t)ry * ry;
	int64_t two_rx_sq = 2 * rx_sq;
	int64_t two_ry_sq = 2 * ry_sq;
	int64_t x = 0;
	int64_t y = ry;
	int64_t px = 0;
	int64_t py = two_rx_sq * y;

	int64_t p = llround(ry_sq - (rx_sq * ry) + (0.25 * rx_sq));
	while (px < py)
	{
		drawFastHLine(x0 - x, y0 + y, 2 * x + 1, color);
		drawFastHLine(x0 - x, y0 - y, 2 * x + 1, color);
		x++;
		px += two_ry_sq;
		if (p < 0)
		{
			p += ry_sq + px;
		}
		else
		{
			y--;
			py -= two_rx_sq;
			p += ry_sq + px - py;
		}
	}

	p = llround(ry_sq * (x + 0.5) * (x + 0.5) + rx_sq * (y - 1) * (y - 1) - rx_sq * ry_sq);
	while (y >= 0)
	{
		drawFastHLine(x0 - x, y0 + y, 2 * x + 1, color);
		drawFastHLine(x0 - x, y0 - y, 2 * x + 1, color);
		y--;
		py -= two_rx_sq;
		if (p > 0)
		{
			p += rx_sq - py;
		}
		else
		{
			x++;
			px += two_ry_sq;
			p += rx_sq - py + px;
		}
	}
}

void UM_GFX_Canvas::drawEllipse(int16_t x0, int16_t y0, int32_t rx, int32_t ry, uint16_t color)
{
	if (rx <= 0 || ry <= 0)
		return;
	int64_t rx_sq = (int64_t)rx * rx;
	int64_t ry_sq = (int64_t)ry * ry;
	int64_t two_rx_sq = 2 * rx_sq;
	int64_t two_ry_sq = 2 * ry_sq;
	int64_t x = 0;
	int64_t y = ry;
	int64_t px = 0;
	int64_t py = two_rx_sq * y;

	int64_t p = llround(ry_sq - (rx_sq * ry) + (0.25 * rx_sq));
	while (px < py)
	{
		drawPixel(x0 + x, y0 + y, color);
		drawPixel(x0 - x, y0 + y, color);
		drawPixel(x0 + x, y0 - y, color);
		drawPixel(x0 - x, y0 - y, color);
		x++;
		px += two_ry_sq;
		if (p < 0)
		{
			p += ry_sq + px;
		}
		else
		{
			y--;
			py -= two_rx_sq;
			p += ry_sq + px - py;
		}
	}

	p = llround(ry_sq * (x + 0.5) * (x + 0.5) + rx_sq * (y - 1) * (y - 1) - rx_sq * ry_sq);
	while (y >= 0)
	{
		drawPixel(x0 + x, y0 + y, color);
		drawPixel(x0 - x, y0 + y, color);
		drawPixel(x0 + x, y0 - y, color);
		drawPixel(x0 - x, y0 - y, color);
		y--;
		py -= two_rx_sq;
		if (p > 0)
		{
			p += rx_sq - py;
		}
		else
		{
			x++;
			px += two_ry_sq;
			p += rx_sq - py + px;
		}
	}
}

void UM_GFX_Canvas::drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4)
		{
			drawPixel(x0 + x, y0 + y, color);
			drawPixel(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2)
		{
			drawPixel(x0 + x, y0 - y, color);
			drawPixel(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8)
		{
			drawPixel(x0 - y, y0 + x, color);
			drawPixel(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1)
		{
			drawPixel(x0 - y, y0 - x, color);
			drawPixel(x0 - x, y0 - y, color);
		}
	}
}

void UM_GFX_Canvas::fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if (corners & 0x1)
		{
			drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
		}
		if (corners & 0x2)
		{
			drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
			drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
		}
	}
}

void UM_GFX_Canvas::fillRoundRect(int x, int y, int w, int h, int r, uint16_t color)
{
	if (w <= 0 || h <= 0)
		return;
	if (r <= 0)
	{
		fillRect(x, y, w, h, color);
		return;
	}
	if (r > std::min(w, h) / 2)
		r = std::min(w, h) / 2;

	fillRect(x + r, y, w - 2 * r, h, color);
	fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + h - r - 1, r, 4, h - 2 * r - 1, color);
	fillCircleHelper(x + w - r - 1, y + h - r - 1, r, 8, h - 2 * r - 1, color);
}

void UM_GFX_Canvas::drawRoundRect(int x, int y, int w, int h, int r, uint16_t color)
{
	if (w <= 0 || h <= 0)
		return;
	if (r <= 0)
	{
		drawRect(x, y, w, h, color);
		return;
	}
	if (r > std::min(w, h) / 2)
		r = std::min(w, h) / 2;

	drawFastHLine(x + r, y, w - 2 * r, color);
	drawFastHLine(x + r, y + h - 1, w - 2 * r, color);
	drawFastVLine(x, y + r, h - 2 * r, color);
	drawFastVLine(x + w - 1, y + r, h - 2 * r, color);

	drawCircleHelper(x + r, y + r, r, 1, color);
	drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

} // namespace umgfx

struct PNGCanvasContext
{
		umgfx::UM_GFX_Canvas *canvas;
		PNG *decoder;
		uint16_t *line_buffer;
		int dest_x;
		int dest_y;
		uint32_t bg_color;
};

static int PNGCanvasDraw(PNGDRAW *pDraw)
{
	if (!pDraw || !pDraw->pUser)
		return 0;

	auto *ctx = static_cast<PNGCanvasContext *>(pDraw->pUser);
	if (!ctx->canvas || !ctx->decoder || !ctx->line_buffer)
		return 0;

	ctx->decoder->getLineAsRGB565(pDraw, ctx->line_buffer, PNG_RGB565_BIG_ENDIAN, ctx->bg_color);

	int dest_y = ctx->dest_y + pDraw->y;
	if (dest_y < 0 || dest_y >= ctx->canvas->height())
		return 0;

	uint16_t *dest = ctx->canvas->data();
	if (!dest)
		return 0;

	dest += dest_y * ctx->canvas->width() + ctx->dest_x;
	memcpy(dest, ctx->line_buffer, pDraw->iWidth * sizeof(uint16_t));

	return 1;
}

static int PNGIgnoreDraw(PNGDRAW *)
{
	return 1;
}

struct JPEGCanvasContext
{
		umgfx::UM_GFX_Canvas *canvas;
};

static int JPEGCanvasDraw(JPEGDRAW *pDraw)
{
	if (!pDraw || !pDraw->pUser)
		return 0;

	auto *ctx = static_cast<JPEGCanvasContext *>(pDraw->pUser);
	if (!ctx || !ctx->canvas || !ctx->canvas->getBuffer())
		return 0;

	umgfx::UM_GFX_Canvas *canvas = ctx->canvas;
	const int canvas_w = canvas->width();
	const int canvas_h = canvas->height();

	if (pDraw->x < 0 || pDraw->y < 0 || pDraw->x + pDraw->iWidth > canvas_w || pDraw->y + pDraw->iHeight > canvas_h)
		return 0;

	uint16_t *dest_base = canvas->data();
	uint16_t *src = reinterpret_cast<uint16_t *>(pDraw->pPixels);
	int line_pixels = pDraw->iWidth;

	for (int row = 0; row < pDraw->iHeight; ++row)
	{
		uint16_t *dest = dest_base + (pDraw->y + row) * canvas_w + pDraw->x;
		memcpy(dest, src + row * line_pixels, static_cast<size_t>(line_pixels) * sizeof(uint16_t));
	}
	return 1;
}

static File g_jpeg_lfs_file;

static void *umgfx_jpegOpenLFS(const char *filename, int32_t *size)
{
	if (!filename)
		return nullptr;
	if (!LittleFS.begin(false))
		return nullptr;
	g_jpeg_lfs_file = LittleFS.open(filename, FILE_READ);
	if (!g_jpeg_lfs_file)
		return nullptr;
	if (size)
		*size = g_jpeg_lfs_file.size();
	return &g_jpeg_lfs_file;
}

static void umgfx_jpegClose(void *handle)
{
	File *file = static_cast<File *>(handle);
	if (file && *file)
		file->close();
}

static int32_t umgfx_jpegRead(JPEGFILE *handle, uint8_t *buffer, int32_t length)
{
	if (!handle || !buffer || length <= 0)
		return 0;
	File *file = static_cast<File *>(handle->fHandle);
	if (!file)
		return 0;
	return file->read(buffer, length);
}

static int32_t umgfx_jpegSeek(JPEGFILE *handle, int32_t position)
{
	if (!handle)
		return 0;
	File *file = static_cast<File *>(handle->fHandle);
	if (!file)
		return 0;
	return file->seek(position);
}

static int JPEGIgnoreDraw(JPEGDRAW *)
{
	return 1;
}

PNGDisplay::PNGDisplay() : _last_error(0) {}

int PNGDisplay::getPNGInfo(int *width, int *height, int *bpp, const void *data, int data_size)
{
	if (!width || !height || !bpp || !data || data_size < 32)
	{
		_last_error = PNG_INVALID_PARAMETER;
		return 0;
	}

	PNG *png = static_cast<PNG *>(psram_alloc(sizeof(PNG)));
	if (!png)
	{
		_last_error = PNG_MEM_ERROR;
		return 0;
	}

	int rc = png->openRAM((uint8_t *)data, data_size, PNGIgnoreDraw);
	if (rc == PNG_SUCCESS)
	{
		*width = png->getWidth();
		*height = png->getHeight();
		*bpp = png->getBpp();
		png->close();
		psram_free(png);
		_last_error = PNG_SUCCESS;
		return 1;
	}

	_last_error = png->getLastError();
	psram_free(png);
	return 0;
}

int PNGDisplay::loadPNG(umgfx::UM_GFX_Canvas *canvas, int x, int y, const void *data, int data_size, uint32_t bgColor)
{
	if (!canvas || !canvas->getBuffer() || !data || data_size <= 0)
	{
		_last_error = PNG_INVALID_PARAMETER;
		return 0;
	}

	PNG *png = static_cast<PNG *>(psram_alloc(sizeof(PNG)));
	if (!png)
	{
		_last_error = PNG_MEM_ERROR;
		return 0;
	}

	int rc = png->openRAM((uint8_t *)data, data_size, PNGCanvasDraw);
	if (rc != PNG_SUCCESS)
	{
		_last_error = png->getLastError();
		psram_free(png);
		return 0;
	}

	int w = png->getWidth();
	int h = png->getHeight();
	if (w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > canvas->width() || y + h > canvas->height())
	{
		png->close();
		psram_free(png);
		_last_error = PNG_INVALID_PARAMETER;
		return 0;
	}

	uint16_t *line = static_cast<uint16_t *>(psram_alloc(static_cast<size_t>(w) * sizeof(uint16_t)));
	if (!line)
	{
		png->close();
		psram_free(png);
		_last_error = PNG_MEM_ERROR;
		return 0;
	}

	PNGCanvasContext ctx{canvas, png, line, x, y, bgColor};

	rc = png->decode(&ctx, 0);
	png->close();
	psram_free(line);
	_last_error = png->getLastError();
	psram_free(png);
	return rc == PNG_SUCCESS;
}

JPEGDisplay::JPEGDisplay() : _last_error(0) {}

int JPEGDisplay::getJPEGInfo(int *width, int *height, int *bpp, const void *data, int data_size)
{
	if (!width || !height || !bpp || !data || data_size < 32)
	{
		_last_error = JPEG_INVALID_PARAMETER;
		return 0;
	}

	JPEGDEC *jpeg = static_cast<JPEGDEC *>(psram_alloc(sizeof(JPEGDEC)));
	if (!jpeg)
	{
		_last_error = JPEG_ERROR_MEMORY;
		return 0;
	}

	if (jpeg->openRAM((uint8_t *)data, data_size, JPEGIgnoreDraw))
	{
		*width = jpeg->getWidth();
		*height = jpeg->getHeight();
		*bpp = jpeg->getBpp();
		jpeg->close();
		psram_free(jpeg);
		_last_error = JPEG_SUCCESS;
		return 1;
	}

	_last_error = jpeg->getLastError();
	psram_free(jpeg);
	return 0;
}

int JPEGDisplay::getJPEGInfo_LFS(int *width, int *height, int *bpp, const char *path)
{
	if (!width || !height || !bpp || !path)
	{
		_last_error = JPEG_INVALID_PARAMETER;
		return 0;
	}

	JPEGDEC *jpeg = static_cast<JPEGDEC *>(psram_alloc(sizeof(JPEGDEC)));
	if (!jpeg)
	{
		_last_error = JPEG_ERROR_MEMORY;
		return 0;
	}

	if (jpeg->open(path, umgfx_jpegOpenLFS, umgfx_jpegClose, umgfx_jpegRead, umgfx_jpegSeek, JPEGIgnoreDraw))
	{
		*width = jpeg->getWidth();
		*height = jpeg->getHeight();
		*bpp = jpeg->getBpp();
		jpeg->close();
		psram_free(jpeg);
		_last_error = JPEG_SUCCESS;
		return 1;
	}

	_last_error = jpeg->getLastError();
	psram_free(jpeg);
	return 0;
}

int JPEGDisplay::loadJPEG(umgfx::UM_GFX_Canvas *canvas, int x, int y, const void *data, int data_size, int options)
{
	if (!canvas || !canvas->getBuffer() || !data || data_size <= 0)
	{
		_last_error = JPEG_INVALID_PARAMETER;
		return 0;
	}

	JPEGDEC *jpeg = static_cast<JPEGDEC *>(psram_alloc(sizeof(JPEGDEC)));
	if (!jpeg)
	{
		_last_error = JPEG_ERROR_MEMORY;
		return 0;
	}

	if (!jpeg->openRAM((uint8_t *)data, data_size, JPEGCanvasDraw))
	{
		_last_error = jpeg->getLastError();
		psram_free(jpeg);
		return 0;
	}

	jpeg->setPixelType(RGB565_BIG_ENDIAN);

	int w = jpeg->getWidth();
	int h = jpeg->getHeight();
	int canvas_w = canvas->width();
	int canvas_h = canvas->height();

	if (x == JPEGDISPLAY_CENTER)
		x = std::max(0, (canvas_w - w) / 2);
	if (y == JPEGDISPLAY_CENTER)
		y = std::max(0, (canvas_h - h) / 2);

	if (w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > canvas_w || y + h > canvas_h)
	{
		jpeg->close();
		psram_free(jpeg);
		_last_error = JPEG_INVALID_PARAMETER;
		return 0;
	}

	JPEGCanvasContext ctx{canvas};
	jpeg->setUserPointer(&ctx);
	int rc = jpeg->decode(x, y, options);
	jpeg->close();
	_last_error = jpeg->getLastError();
	psram_free(jpeg);
	return rc == JPEG_SUCCESS;
}

int JPEGDisplay::loadJPEG_LFS(umgfx::UM_GFX_Canvas *canvas, int x, int y, const char *path, int options)
{
	if (!canvas || !canvas->getBuffer() || !path)
	{
		_last_error = JPEG_INVALID_PARAMETER;
		return 0;
	}

	JPEGDEC *jpeg = static_cast<JPEGDEC *>(psram_alloc(sizeof(JPEGDEC)));
	if (!jpeg)
	{
		_last_error = JPEG_ERROR_MEMORY;
		return 0;
	}

	if (!jpeg->open(path, umgfx_jpegOpenLFS, umgfx_jpegClose, umgfx_jpegRead, umgfx_jpegSeek, JPEGCanvasDraw))
	{
		_last_error = jpeg->getLastError();
		psram_free(jpeg);
		return 0;
	}

	jpeg->setPixelType(RGB565_BIG_ENDIAN);

	int w = jpeg->getWidth();
	int h = jpeg->getHeight();
	int canvas_w = canvas->width();
	int canvas_h = canvas->height();

	if (x == JPEGDISPLAY_CENTER)
		x = std::max(0, (canvas_w - w) / 2);
	if (y == JPEGDISPLAY_CENTER)
		y = std::max(0, (canvas_h - h) / 2);

	if (w <= 0 || h <= 0 || x < 0 || y < 0 || x + w > canvas_w || y + h > canvas_h)
	{
		jpeg->close();
		psram_free(jpeg);
		_last_error = JPEG_INVALID_PARAMETER;
		return 0;
	}

	JPEGCanvasContext ctx{canvas};
	jpeg->setUserPointer(&ctx);
	int rc = jpeg->decode(x, y, options);
	jpeg->close();
	_last_error = jpeg->getLastError();
	psram_free(jpeg);
	return rc == JPEG_SUCCESS;
}
