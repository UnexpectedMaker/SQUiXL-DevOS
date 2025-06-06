#pragma once
#include <functional>
#include "bb_spi_lcd.h"

using ScreenieCallback = std::function<void(bool)>;

struct ScreenieState
{
		BB_SPI_LCD *lcd = nullptr;
		uint8_t *rgb = nullptr;
		size_t out_idx = 0;
		uint32_t y = 0;
		uint32_t rows_per_step = 10;
		bool ready_to_encode = false;
		ScreenieCallback cb = nullptr;
		bool running = false;
};

// Global instance
extern ScreenieState screenie;

bool screenie_start(BB_SPI_LCD *lcd, ScreenieCallback cb, uint32_t rows_per_step = 10);
void screenie_tick();