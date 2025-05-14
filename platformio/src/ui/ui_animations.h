#ifndef UI_ANIMATIONS_H
#define UI_ANIMATIONS_H

#include <Arduino.h>
#include <functional>
#include <vector>

enum tween_ease_t
{
	EASE_LINEAR,
	EASE_IN,	// ease-in: slow start, then speeds up (quadratic)
	EASE_OUT,	// ease-out: fast start, then slows down (quadratic)
	EASE_IN_OUT // ease-in-out: slow start and finish, faster in the middle
};

// Abstract base class for animations.
class ui_animation
{
	public:
		// update() returns true if the animation should continue running.
		virtual bool update() = 0;
		virtual ~ui_animation() {}
};

// A generic tween animation for float values.
class tween_animation : public ui_animation
{
	private:
		float start_value, end_value;
		tween_ease_t ease;
		unsigned long duration;						// Duration in milliseconds.
		unsigned long start_time;					// Time when the animation started.
		std::function<void(float)> update_callback; // Callback to update the property.
		std::function<void()> completion_callback;	// Optional callback when done.
		bool play_reverse;
		bool started;

	public:
		tween_animation(float start, float end, unsigned long duration_ms, tween_ease_t ease_type, std::function<void(float)> update_cb, std::function<void()> completion_cb = nullptr);
		bool update() override;
};

// Manager that holds and updates all animations.
class ui_animation_manager
{
	private:
		std::vector<ui_animation *> animations;
		SemaphoreHandle_t mutex; // Mutex to protect the animations vector.

	public:
		ui_animation_manager()
		{
			mutex = xSemaphoreCreateMutex();
		}

		~ui_animation_manager()
		{
			if (mutex)
			{
				vSemaphoreDelete(mutex);
			}
		}

		// Adds a new animation.
		void add_animation(ui_animation *anim);
		// Calls update on all animations and cleans up finished ones.
		void update_animations();

		int active_animations() { return animations.size(); }
};

// Global animation manager instance.
extern ui_animation_manager animation_manager;

// // Example UI element classes:

// // A label that can update its brightness.
// class ui_label
// {
// 	public:
// 		void set_brightness(int brightness);
// };

// // An icon that can change its opacity and image.
// class ui_icon
// {
// 	public:
// 		void set_opacity(float opacity);
// 		void set_image(const String &image_name);
// };

// // A window that can fade in/out and contains child UI elements.
// class ui_window
// {
// 	public:
// 		std::vector<ui_icon *> children; // For example, child icons.
// 		void set_opacity(float opacity);
// };

// // A generic UI element that can have its position animated.
// class ui_element
// {
// 	public:
// 		void set_x(int x);
// };

// Utility functions to animate common UI behaviors:

// // Fades out an icon, changes its image, then fades it back in.
// void animate_icon(ui_icon *icon, const String &new_image);

// // Fades in a window (and its children).
// void fade_in_window(ui_window *window);

// // Animates a UI element's X position from start_x to end_x.
// void animate_position(ui_element *element, int start_x, int end_x, unsigned long duration);

// FreeRTOS task function to update animations.
void animation_task_loop(void *param);

#endif // UI_ANIMATIONS_H