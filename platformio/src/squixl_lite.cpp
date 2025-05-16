/*
	Base library for SQUiXL
	Copyright (c) 2025 Unexpected Maker
	https://squixl.io

	version 0.1
*/

#include "squixl_lite.h"

void SQUiXL_LITE::init()
{
	Serial.println("Starting screen init");

	/* Initialise LCA9555 IO Expander */
	if (ioex.begin(0x20))
	{
		// LCD Reset Line
		ioex.pin_mode(LCD_RST, OUTPUT, HIGH);

		// Screen backlight EN
		ioex.pin_mode(BL_EN, OUTPUT, HIGH);

		// Screen soft power EN
		// ioex.pin_mode(SOFT_PWR, OUTPUT, LOW);

		ioex.pin_mode(VBUS_SENSE, INPUT);

		// IO MUX - EN is Active LOW, so start it off
		ioex.pin_mode(MUX_EN, OUTPUT, HIGH);
		// IO MUX - Set default to I2S - LOW is SD
		ioex.pin_mode(MUX_SEL, OUTPUT, HIGH);

		// Haptic EN
		ioex.pin_mode(HAPTICS_EN, OUTPUT, HIGH);

		// Touch RESET
		ioex.pin_mode(TP_RST, OUTPUT, LOW);
		delay(100);

		xtouch.begin(TP_INT, TP_RST);

		Serial.println("LCA9555 is Good!");
	}
	else
	{
		Serial.println("LCA9555 Init Failed");
	}

	/* Display */

	// First we need to init the LCD via IOEX bitbanging (Not in current PCB version)
	screen_init_spi_bitbanged(st7701s_init_commands);

	// Now we can init the display via the ESP32-S3 RGB Peripheral
	lcd.begin(DISPLAY_UM_480x480); // initialize the display
	int buf_error = lcd.allocBuffer();

	if (buf_error == -2)
	{
		// unable to allocate a buffer
		Serial.println("allocBuffer() already alloc!!");
	}
	else if (buf_error == -1)
	{
		Serial.println("allocBuffer() failed!");
		while (1)
		{
		}; // stop
	}
	else
	{
		Serial.println("Display buffer allocated successfully!");
	}

	// Set the SPI frequency for the screen to 6Mhz to remove contention between PSRAM (frame buffer) and Flash.
	RGBChangeFreq(6000000);
}

void SQUiXL_LITE::screen_init_spi_bitbanged(const uint8_t *data)
{

	const uint8_t MOSI = 2; // P02
	const uint8_t CLK = 3;	// P02
	const uint8_t CS = 4;	// P03

	// Configure pins as output
	ioex.pin_mode(MOSI, OUTPUT);
	ioex.pin_mode(CLK, OUTPUT);
	ioex.pin_mode(CS, OUTPUT);

	// Set initial pin states
	ioex.write(CS, HIGH);
	ioex.write(CLK, LOW);

	ioex.write(MOSI, LOW);

	int bytes_written = 0;

	while (*data)
	{
		uint8_t len = *data++;
		if (len == LCD_DELAY)
		{
			delay(*data++);
			continue;
		}
		uint8_t cmd = *data++;

		// Start SPI transaction
		ioex.write(CS, LOW);

		// Clock DC bit for command
		ioex.write(CLK, LOW);
		ioex.write(MOSI, 0);
		ioex.write(CLK, HIGH);

		// Send command
		for (int i = 7; i >= 0; i--)
		{
			ioex.write(CLK, LOW);
			ioex.write(MOSI, (cmd >> i) & 0x01);
			ioex.write(CLK, HIGH);
		}

		// Send data bytes
		for (uint8_t i = 0; i < len - 1; i++)
		{
			uint8_t byte = *data++;

			// Clock DC bit for data
			ioex.write(CLK, LOW);
			ioex.write(MOSI, 1);
			ioex.write(CLK, HIGH);

			for (int j = 7; j >= 0; j--)
			{
				ioex.write(CLK, LOW);
				ioex.write(MOSI, (byte >> j) & 0x01);
				ioex.write(CLK, HIGH);
			}
		}

		// End SPI transaction
		ioex.write(CS, HIGH);
	}
}

/*
IO MUX Stuff
*/
void SQUiXL_LITE::mux_toggle()
{
	if (millis() - next_mux_switch < 500)
		return;

	next_mux_switch = millis();

	if (_current_mux_state == MUX_STATE::MUX_I2S)
		mux_switch_to(MUX_STATE::MUX_SD);
	else
		mux_switch_to(MUX_STATE::MUX_I2S);
}

bool SQUiXL_LITE::mux_check_state(MUX_STATE new_state)
{
	return (_current_mux_state == new_state);
}

bool SQUiXL_LITE::mux_switch_to(MUX_STATE new_state)
{
	if (new_state == MUX_STATE::MUX_I2S && _current_mux_state == MUX_STATE::MUX_I2S)
	{
		Serial.println("I2S Already Selected!");
		return false;
	}
	else if (new_state == MUX_STATE::MUX_SD && _current_mux_state == MUX_STATE::MUX_SD)
	{
		Serial.println("SD Already Selected!");
		return false;
	}
	else if (new_state == MUX_STATE::MUX_OFF && _current_mux_state == MUX_STATE::MUX_OFF)
	{
		Serial.println("MUX Already Off");
		return false;
	}

	// We are switching, so shutdown whatever is current
	if (_current_mux_state == MUX_STATE::MUX_I2S)
	{
#ifdef AUDIO_AVAILABLE
		audio.stop();
#endif
		Serial.println("Stopped I2S Peripheral");
	}
	else if (_current_mux_state == MUX_STATE::MUX_SD)
	{
		SD.end();
		SPI.end();
		Serial.println("Stopped SD");
	}
	else if (_current_mux_state == MUX_STATE::MUX_OFF)
	{
		// Re-enable the IO MNUX
		Serial.println("Enabled MUX");
		ioex.write(MUX_EN, LOW);
	}

	delay(50);

	// Now do teh switch
	if (new_state == MUX_STATE::MUX_I2S)
	{
		ioex.write(MUX_SEL, HIGH); // HIGH is I2S, LOW is SD
		delayMicroseconds(1);
#ifdef AUDIO_AVAILABLE
		audio.setup(MUX_D3, MUX_D4, MUX_D2, MUX_D1);
#endif
		delay(10);
		Serial.println("Switched to I2S via MUX");
		_current_mux_state = MUX_STATE::MUX_I2S;
	}
	else if (new_state == MUX_STATE::MUX_SD)
	{
		ioex.write(MUX_SEL, LOW); // HIGH is I2S, LOW is SD
		delay(10);

		Serial.println("Switched to SD via MUX");

		SPI.begin(MUX_D3, MUX_D1, MUX_D4, MUX_D2);
		if (!SD.begin(MUX_D2))
		{

			Serial.println("Card Mount Failed");
			return false;
		}
		uint8_t cardType = SD.cardType();

		if (cardType == CARD_NONE)
		{
			Serial.println("No SD card attached");
			return false;
		}

		Serial.print("SD Card Type: ");
		if (cardType == CARD_MMC)
		{
			Serial.println("MMC");
		}
		else if (cardType == CARD_SD)
		{
			Serial.println("SDSC");
		}
		else if (cardType == CARD_SDHC)
		{
			Serial.println("SDHC");
		}
		else
		{
			Serial.println("UNKNOWN");
		}

		uint64_t cardSize = SD.cardSize() / (1024 * 1024);
		Serial.printf("SD Card Size: %lluMB\n", cardSize);

		// Setup SD card stuff here

		_current_mux_state = MUX_STATE::MUX_SD;
	}
	else if (new_state == MUX_STATE::MUX_OFF)
	{
		// Disable the IO MNUX
		ioex.write(MUX_EN, HIGH);
		Serial.println("Disabled MUX");

		_current_mux_state = MUX_STATE::MUX_OFF;
	}

	next_touch = millis();

	return true;
}

bool SQUiXL_LITE::vbus_present()
{
	return (is_5V_detected);
}

void SQUiXL_LITE::cache_text_sizes()
{
	// If the size is more than 0 we have alrady cached, so exit.
	if (font_char_sizes.size() > 0)
		return;

	int16_t tempx;
	int16_t tempy;
	uint16_t tempw;
	uint16_t temph;

	BB_SPI_LCD _font_checker;

	// Regular
	for (int i = 0; i < UbuntuMono_R_count; i++)
	{
		_font_checker.setFreeFont(UbuntuMono_R[i]);
		_font_checker.getTextBounds("W", 0, 0, &tempx, &tempy, &tempw, &temph);
		font_size_t size;
		size.weight = FONT_SPEC::FONT_WEIGHT_R;
		size.size = i;
		size.width = tempw;
		size.height = temph;
		font_char_sizes.push_back(size);
	}

	// Bold
	for (int i = 0; i < UbuntuMono_B_count; i++)
	{
		_font_checker.setFreeFont(UbuntuMono_B[i]);
		_font_checker.getTextBounds("W", 0, 0, &tempx, &tempy, &tempw, &temph);
		font_size_t size;
		size.weight = FONT_SPEC::FONT_WEIGHT_R;
		size.size = i;
		size.width = tempw;
		size.height = temph;
		font_char_sizes.push_back(size);
	}

	Serial.printf("Added %d cached char sizes for UbuntuMono_R & Ubuntu_Mono_B\n", font_char_sizes.size());
}

void SQUiXL_LITE::get_cached_char_sizes(FONT_SPEC weight, uint8_t size, uint8_t *width, uint8_t *height)
{
	*width = 0;
	*height = 0;

	for (int i = 0; i < font_char_sizes.size(); i++)
	{
		// font_char_sizes[i].print();
		if (font_char_sizes[i].weight == (int)weight && font_char_sizes[i].size == size)
		{
			*width = font_char_sizes[i].width;
			*height = font_char_sizes[i].height;
			return;
			// Serial.println("**found**");
		}
	}
}

void SQUiXL_LITE::show_error(String error, bool fade)
{
	BB_SPI_LCD sprite;
	sprite.createVirtual(480, 480, NULL, true);

	// Cach the font sizes (if required) so we can easily source any sized character wisth and height
	cache_text_sizes();

	uint8_t char_width = 0;
	uint8_t char_height = 0;
	// Load char_width & char_height with the cached sized for Regular sized 3
	get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 3, &char_width, &char_height);
	sprite.fillScreen(TFT_RED);
	sprite.setFreeFont(UbuntuMono_R[3]);
	sprite.setTextColor(TFT_WHITE, -1);

	uint16_t pixels = error.length() * char_width;
	// Set the text to the center of the screen
	sprite.setCursor(240 - pixels / 2, 240 + char_height / 2);
	sprite.print(error);

	if (fade)
	{
		for (uint8_t alpha = 0; alpha < 32; alpha++)
		{
			squixl_lite.lcd.blendSprite(&sprite, &squixl_lite.lcd, &squixl_lite.lcd, alpha);
			delay(25);
		}
	}
	else
	{
		squixl_lite.lcd.drawSprite(0, 0, &sprite, 1.0, -1);
	}
	sprite.freeBuffer();
}

bool SQUiXL_LITE::process_touch_lite(uint16_t *x, uint16_t *y)
{
	if (millis() - next_touch < touch_rate)
		return false;

	next_touch = millis();

	uint16_t pts[5][4];
	uint8_t n = xtouch.readPoints(pts);

	if (n > 0)
	{
		*x = pts[0][0];
		*y = pts[0][1];
		return true;
	}

	return false;
}

SQUiXL_LITE squixl_lite;