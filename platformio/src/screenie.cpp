#include <vector>
#include <utils/lodepng.h>
#include <LittleFS.h>
#include "bb_spi_lcd.h"
#include "settings/settings.h"

#include <cmath>
#include <algorithm>
#include <cstdint>

inline uint8_t percent_to_255(float pct)
{
	return uint8_t(std::clamp(pct, 0.0f, 100.0f) * 255.0f / 100.0f + 0.5f);
}

inline uint8_t norm_to_255(float norm)
{
	return uint8_t(std::clamp(norm, 0.0f, 1.0f) * 255.0f + 0.5f);
}

uint8_t apply_levels(uint8_t input, uint8_t in_black, uint8_t in_white, float gamma, uint8_t out_black, uint8_t out_white)
{
	float clamped = std::clamp(float(input), float(in_black), float(in_white));

	// 2. Normalize input to [0,1] range
	float normalized = (clamped - float(in_black)) / float(in_white - in_black);

	// 3. Clamp normalized to [0,1] before gamma
	normalized = std::clamp(normalized, 0.0f, 1.0f);

	// 4. Apply the Photoshop/Affinity gamma
	// float gamma_corrected = std::pow(normalized, 1.0f / gamma);
	float gamma_corrected = std::pow(normalized, gamma);

	// 5. Map back to output range
	float output = float(out_black) + gamma_corrected * float(out_white - out_black);

	// 6. Clamp to 0-255 and return
	return static_cast<uint8_t>(std::clamp(output, 0.0f, 255.0f));
}

uint8_t adjust_contrast(uint8_t val, float contrast)
{
	int out = int((float(val) - 128.0f) * contrast + 128.0f + 0.5f);
	return std::clamp(out, 0, 255);
}

// RGB [0-255] to HSL [0-1]
void rgb_to_hsl(uint8_t r, uint8_t g, uint8_t b, float &h, float &s, float &l)
{
	float rf = r / 255.0f, gf = g / 255.0f, bf = b / 255.0f;
	float max = std::max(rf, std::max(gf, bf)), min = std::min(rf, std::min(gf, bf));
	l = (max + min) / 2.0f;
	if (max == min)
	{
		h = s = 0.0f;
	}
	else
	{
		float d = max - min;
		s = (l > 0.5f) ? d / (2.0f - max - min) : d / (max + min);
		if (max == rf)
			h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);
		else if (max == gf)
			h = (bf - rf) / d + 2.0f;
		else
			h = (rf - gf) / d + 4.0f;
		h /= 6.0f;
	}
}

// HSL [0-1] to RGB [0-255]
void hsl_to_rgb(float h, float s, float l, uint8_t &r, uint8_t &g, uint8_t &b)
{
	auto hue2rgb = [](float p, float q, float t) {
		if (t < 0.0f)
			t += 1.0f;
		if (t > 1.0f)
			t -= 1.0f;
		if (t < 1.0f / 6.0f)
			return p + (q - p) * 6.0f * t;
		if (t < 1.0f / 2.0f)
			return q;
		if (t < 2.0f / 3.0f)
			return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		return p;
	};
	float r1, g1, b1;
	if (s == 0)
		r1 = g1 = b1 = l;
	else
	{
		float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
		float p = 2.0f * l - q;
		r1 = hue2rgb(p, q, h + 1.0f / 3.0f);
		g1 = hue2rgb(p, q, h);
		b1 = hue2rgb(p, q, h - 1.0f / 3.0f);
	}
	r = uint8_t(std::clamp(r1 * 255.0f, 0.0f, 255.0f));
	g = uint8_t(std::clamp(g1 * 255.0f, 0.0f, 255.0f));
	b = uint8_t(std::clamp(b1 * 255.0f, 0.0f, 255.0f));
}

// Apply saturation exactly as in Photoshop/Affinity
void adjust_saturation(uint8_t &r, uint8_t &g, uint8_t &b, float sat_factor)
{
	float h, s, l;
	rgb_to_hsl(r, g, b, h, s, l);
	s *= sat_factor;
	s = std::clamp(s, 0.0f, 1.0f);
	hsl_to_rgb(h, s, l, r, g, b);
}
void apply_affinity_style_white_balance(uint8_t &r, uint8_t &g, uint8_t &b, float temp, float tint)
{
	// --- Temperature ---
	// "Cool" boosts blue, "warm" boosts red (with some green)
	float red = r;
	float green = g;
	float blue = b;

	// These factors control strength -- tweak to taste!
	float temp_strength = 60.0f; // Max 60 out of 255 for full slider swing
	float tint_strength = 40.0f; // Max 40 out of 255 for full slider swing

	// Temperature: shift red/blue
	red += temp * temp_strength;
	blue -= temp * temp_strength;

	// Tint: Affinity/Photoshop - right (positive) = magenta, left (negative) = green
	green -= tint * tint_strength;
	red += tint * (tint_strength / 2.0f);
	blue += tint * (tint_strength / 2.0f);

	// Clamp
	r = std::clamp(int(red), 0, 255);
	g = std::clamp(int(green), 0, 255);
	b = std::clamp(int(blue), 0, 255);
}

bool save_png(BB_SPI_LCD *screen)
{
	// 1) mount LittleFS
	if (!LittleFS.begin())
	{
		Serial.println("❌ LittleFS mount failed");
		return false;
	}

	// 2) grab a pointer to the raw RGB565 buffer
	const uint8_t *raw = reinterpret_cast<const uint8_t *>(screen->getBuffer());
	const uint32_t W = 480, H = 480;

	// // 3) expand into RGB888 in a single pre-sized array
	// std::vector<uint8_t> rgb;
	// rgb.resize(W * H * 3);
	size_t outIdx = 0;

	size_t pxCount = (size_t)W * H;
	auto rgb = (uint8_t *)heap_caps_malloc(
		pxCount * 3,
		MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
	);

	uint8_t black = norm_to_255(settings.config.screenshot.black);
	uint8_t white = norm_to_255(settings.config.screenshot.white);

	for (uint32_t y = 0; y < H; y++)
	{
		for (uint32_t x = 0; x < W; x++)
		{
			size_t inIdx = (y * W + x) * 2;
			// assemble big-endian 565:
			uint16_t pix565 = (uint16_t(raw[inIdx]) << 8) | raw[inIdx + 1];

			uint8_t r = ((pix565 >> 11) & 0x1F);
			r = (r << 3) | (r >> 2);

			uint8_t g = ((pix565 >> 5) & 0x3F);
			g = (g << 2) | (g >> 4);

			uint8_t b = (pix565 & 0x1F);
			b = (b << 3) | (b >> 2);

			if (settings.config.screenshot.gamma != 1.0 || settings.config.screenshot.black > 0.0 || settings.config.screenshot.white < 1.0)
			{
				r = apply_levels(r, black, white, settings.config.screenshot.gamma, 0, 255);
				g = apply_levels(g, black, white, settings.config.screenshot.gamma, 0, 255);
				b = apply_levels(b, black, white, settings.config.screenshot.gamma, 0, 255);
			}

			if (settings.config.screenshot.saturation != 1.0)
			{
				adjust_saturation(r, g, b, settings.config.screenshot.saturation);
			}

			if (settings.config.screenshot.contrast != 1.0)
			{
				r = adjust_contrast(r, settings.config.screenshot.contrast);
				g = adjust_contrast(g, settings.config.screenshot.contrast);
				b = adjust_contrast(b, settings.config.screenshot.contrast);
			}

			if (settings.config.screenshot.temperature != 1.0 || settings.config.screenshot.tint != 1.0)
			{
				apply_affinity_style_white_balance(r, g, b, settings.config.screenshot.temperature, settings.config.screenshot.tint);
			}

			rgb[outIdx++] = r;
			rgb[outIdx++] = g;
			rgb[outIdx++] = b;
		}
	}

	// 4) encode with LodePNG
	std::vector<uint8_t> png;
	unsigned err = lodepng::encode(
		png, // output
		rgb, // input
		W, H,
		LCT_RGB, // color type
		8		 // bits per channel
	);

	heap_caps_free(rgb);
	rgb = nullptr; // (optional, to avoid dangling pointer)

	if (err)
	{
		Serial.printf("❌ PNG encode failed (%u): %s\n", err, lodepng_error_text(err));
		return false;
	}

	// 5) write file
	File f = LittleFS.open("/screenshot.png", FILE_WRITE);
	if (!f)
	{
		Serial.println("❌ Failed to open /screenshot.png for write");
		return false;
	}
	f.write(png.data(), png.size());
	f.close();

	Serial.printf("✅ Wrote screenshot.png (%u bytes)\n", (unsigned)png.size());

	return false;
}