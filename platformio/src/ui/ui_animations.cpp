#include "ui_animations.h"

// ----------------------
// tween_animation Methods
// ----------------------
tween_animation::tween_animation(float start, float end, unsigned long duration_ms, tween_ease_t ease_type, std::function<void(float)> update_cb, std::function<void()> completion_cb)
	: start_value(start), end_value(end), duration(duration_ms), ease(ease_type),
	  update_callback(update_cb), completion_callback(completion_cb), started(false)
{
}

bool tween_animation::update()
{
	if (!started)
	{
		start_time = millis();
		started = true;
	}
	unsigned long now = millis();
	float t = (now - start_time) / (float)duration;
	if (t >= 1.0f)
	{
		t = 1.0f;
	}

	// Choose your easing function based on an 'ease' member.
	float eased_t;
	switch (ease)
	{
	case EASE_LINEAR:
		eased_t = t;
		break;
	case EASE_IN:
		eased_t = t * t;
		break;
	case EASE_OUT:
		eased_t = t * (2 - t);
		break;
	case EASE_IN_OUT:
		eased_t = (t < 0.5f) ? (2 * t * t) : (-1 + (4 - 2 * t) * t);
		break;
	default:
		eased_t = t;
		break;
	}

	float current_value = start_value + (end_value - start_value) * eased_t;

	// Instead of taskYIELD here, simply update the value.
	update_callback(current_value);

	if (t >= 1.0f)
	{
		if (completion_callback)
		{
			completion_callback();
			// Avoid forcing a yield here.
		}
		return false; // Animation complete.
	}
	return true; // Continue animation.
}

// ---------------------------
// ui_animation_manager Methods
// ---------------------------
void ui_animation_manager::add_animation(ui_animation *anim)
{
	// Take the mutex before modifying the vector.
	if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
	{
		animations.push_back(anim);
		xSemaphoreGive(mutex);
	}
}

void ui_animation_manager::update_animations()
{
	// Take the mutex before iterating over the vector.
	if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)
	{
		for (auto it = animations.begin(); it != animations.end();)
		{
			if (!(*it)->update())
			{ // If update returns false, animation is finished.
				delete *it;
				it = animations.erase(it);
			}
			else
			{
				++it;
			}
			// taskYIELD();
		}
		xSemaphoreGive(mutex);
	}
}

// Global instance of the animation manager.
ui_animation_manager animation_manager;

// // ----------------------
// // UI Element Implementations
// // ----------------------
// void ui_label::set_brightness(int brightness)
// {
// 	// Replace with your actual UI update code.
// 	Serial.println("ui_label brightness: " + String(brightness));
// }

// void ui_icon::set_opacity(float opacity)
// {
// 	// Replace with your actual display update code.
// 	Serial.println("ui_icon opacity: " + String(opacity));
// }

// void ui_icon::set_image(const String &image_name)
// {
// 	// Code to change the icon's image.
// 	Serial.println("ui_icon image changed to: " + image_name);
// }

// void ui_window::set_opacity(float opacity)
// {
// 	// Update the window's opacity.
// 	Serial.println("ui_window opacity: " + String(opacity));
// 	// Also update each child element.
// 	for (auto child : children)
// 	{
// 		if (child)
// 		{
// 			child->set_opacity(opacity);
// 		}
// 	}
// }

// void ui_element::set_x(int x)
// {
// 	// Update the X position of the element.
// 	Serial.println("ui_element X position: " + String(x));
// }

// ----------------------
// Animation Utility Functions
// ----------------------

// // Animate an icon to fade out, change its image, and fade back in.
// void animate_icon(ui_icon *icon, const String &new_image)
// {
// 	animation_manager.add_animation(new tween_animation(1.0, 0.0, 500, [icon](float opacity) { icon->set_opacity(opacity); }, [icon, new_image]() {
//             // On fade-out completion, change image and start fade-in.
//             icon->set_image(new_image);
//             animation_manager.add_animation(new tween_animation(0.0, 1.0, 500,
//                 [icon](float opacity) {
//                     icon->set_opacity(opacity);
//                 }
//             )); }));
// }

// // Fade in a window (and its children) from 0.0 to 1.0 opacity.
// void fade_in_window(ui_window *window)
// {
// 	animation_manager.add_animation(new tween_animation(0.0, 1.0, 1000, [window](float opacity) {
// 		window->set_opacity(opacity);
// 	}));
// }

// // Animate a ui_element's X position from start_x to end_x.
// void animate_position(ui_element *element, int start_x, int end_x, unsigned long duration)
// {
// 	animation_manager.add_animation(new tween_animation((float)start_x, (float)end_x, duration, [element](float x) {
// 		element->set_x((int)x);
// 	}));
// }

// ----------------------
// Animation Task (FreeRTOS)
// ----------------------
void animation_task_loop(void *param)
{
	while (true)
	{
		animation_manager.update_animations();
		// Yield control for 10ms between updates.
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
