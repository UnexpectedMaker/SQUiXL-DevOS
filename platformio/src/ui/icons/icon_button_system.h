#pragma once

#include "ui/ui_icon.h"

class button_icon_system : public ui_icon
{
	public:
		bool process_touch(touch_event_t touch_event) override;

	private:
};

extern button_icon_system button_system;