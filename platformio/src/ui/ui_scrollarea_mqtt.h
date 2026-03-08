#pragma once
#include "ui/ui_scrollarea.h"

class ui_scrollarea_mqtt : public ui_scrollarea
{
	protected:
		bool external_content_dirty() const override;
		void render_content() override;
		void after_content_render() override;
};
