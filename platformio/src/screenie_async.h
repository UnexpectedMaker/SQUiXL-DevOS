#pragma once
#include <functional>
#include "UM_GFX.h"

using ScreenieCallback = std::function<void(bool)>;

struct ScreenieState
{
		umgfx::UM_GFX_Canvas *lcd = nullptr;
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

bool screenie_start(umgfx::UM_GFX_Canvas *lcd, ScreenieCallback cb, uint32_t rows_per_step = 10);
void screenie_tick();
