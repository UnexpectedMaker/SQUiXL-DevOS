# UI System Overview

## Display Hardware & Driver

### Physical Display
- 480x480 pixel RGB LCD panel (ST7701S controller)
- ESP32-S3 RGB peripheral drives the panel via DMA
- Framebuffer operates in **big-endian RGB565** format

### Graphics Library: UM_GFX (`lib/UM_GFX/`)

Two main classes:

1. **`UM_GFX`** - The physical display driver
   - Inherits from `UM_GFX_Canvas`
   - Manages the RGB panel via ESP-IDF `esp_lcd_panel_rgb`
   - Owns the framebuffer in PSRAM
   - Handles vsync, backlight, sleep
   - Global instance: `squixl.lcd`

2. **`UM_GFX_Canvas`** - Off-screen sprite/buffer
   - Used for all off-screen rendering
   - `create(w, h, fill_color)` allocates in PSRAM, optionally fills with color
   - `createWithBuffer(w, h, buffer)` uses external buffer (e.g., DMA framebuffer)
   - `release()` frees the buffer if owned
   - Drawing primitives: `fillRect`, `drawSprite`, `blendSprite`, etc.
   - Text rendering via GFXfont (Adafruit format)

### Key Graphics Operations

```cpp
// Create a sprite (allocates in PSRAM)
umgfx::UM_GFX_Canvas sprite;
sprite.create(width, height);           // No fill (uninitialized)
sprite.create(width, height, TFT_BLACK); // Fill with black
sprite.create(width, height, TFT_MAGENTA); // Fill with transparency color

// Draw sprite to another sprite or LCD
dest.drawSprite(x, y, &sprite, scale, transparent_color);

// Blend same-size sprites with alpha (0-32 range, 32 = fully opaque)
UM_GFX_Canvas::blendSprite(&foreground, &background, &dest, alpha, transparent_color);

// Blend smaller sprite at position (SIMD accelerated)
UM_GFX_Canvas::blendSpriteAt(&foreground, &dest, x, y, alpha, transparent_color);

// Release when done
sprite.release();
```

Transparency color is typically `TFT_MAGENTA` (0xF81F).

---

## UI Class Hierarchy

```
ui_element (base)
├── ui_screen           (full-screen container)
├── ui_window           (floating window container)
├── ui_dialogbox        (modal dialog with OK/Cancel buttons)
├── ui_keyboard         (on-screen keyboard for text input)
├── ui_control          (interactive control base)
│   ├── ui_control_button
│   ├── ui_control_toggle
│   ├── ui_control_slider
│   ├── ui_control_textbox
│   └── ui_control_tabgroup
├── ui_label            (static text)
├── ui_icon             (static image)
├── ui_scrollarea       (scrollable container)
│   ├── ui_scrollarea_wifimanager  (WiFi network list)
│   └── ui_scrollarea_mqtt         (MQTT message list)
└── widget_*            (data-driven widgets)
    ├── widgetTime
    ├── widgetBattery
    ├── widgetOpenWeather
    ├── widgetRSSFeeds
    ├── widgetJokes
    ├── widgetBME280
    ├── widgetMQTTSensors
    └── widgetWifiManager
```

---

## ui_element (Base Class)

Every UI component inherits from `ui_element`. Key members:

### Sprites (off-screen buffers)
```cpp
umgfx::UM_GFX_Canvas _sprite_content;  // Main content drawing
umgfx::UM_GFX_Canvas _sprite_clean;    // Captured background
umgfx::UM_GFX_Canvas _sprite_back;     // Background layer (screens)
umgfx::UM_GFX_Canvas _sprite_mixed;    // Composited output
```

### Position & Size
```cpp
int16_t _x, _y;      // Position
int16_t _w, _h;      // Dimensions
```

### State Flags
```cpp
bool is_dirty;       // Needs redraw
bool is_dirty_hard;  // Needs background recapture
bool is_busy;        // Currently drawing (prevents re-entry)
bool has_never_drawn;
```

### Key Virtual Methods
```cpp
virtual bool redraw(uint8_t fade_amount, int8_t tab_group = -1);
virtual bool process_touch(touch_event_t touch_event);
virtual void about_to_show_screen();
virtual void about_to_close_screen();
```

### Refresh System
```cpp
uint16_t refresh_interval;  // ms between auto-refreshes (0 = manual only)
unsigned long next_refresh;

bool should_refresh();      // Checks if refresh_interval elapsed
```

### Parent/Child Relationships
```cpp
ui_element *ui_parent;
std::vector<ui_element *> ui_children;
std::map<int8_t, std::vector<ui_element *>> tab_group_children;  // For tabbed UIs
```

---

## ui_screen (Full Screen Container)

A `ui_screen` represents a complete 480x480 screen. Key responsibilities:

### Buffers
- `_sprite_back` - Background image (wallpaper JPG)
- `_sprite_content` - All child elements drawn here with TFT_MAGENTA transparency
- `_sprite_drag` - Used during screen-to-screen drag transitions

### Navigation
Screens link to neighbors for swipe navigation:
```cpp
ui_screen *navigation[4];  // UP=0, RIGHT=1, DOWN=2, LEFT=3

void set_navigation(Directions from, ui_screen *screen, bool set_reversed);
```

**Direction enum:** `UP=0, RIGHT=1, DOWN=2, LEFT=3, NONE=99`

**`set_reversed`:** When true, automatically sets up the reverse link on the target screen. The reverse direction is calculated as `((int)from - 2) & 3` (UP↔DOWN, LEFT↔RIGHT). Example:
```cpp
screen_main->set_navigation(Directions::LEFT, screen_mqtt, true);
// Also sets: screen_mqtt->navigation[RIGHT] = screen_main
```

**Drag direction semantics:** Dragging LEFT reveals `navigation[LEFT]` sliding in from the right side. The direction refers to which screen you reach, not which way the content slides.

### Tab Groups
A screen can have a `ui_control_tabgroup` to organize controls into tabs:
```cpp
ui_control_tabgroup *ui_tab_group;

void set_page_tabgroup(ui_control_tabgroup *tabgroup);
int8_t get_tab_group_index();
```

### Key Methods
```cpp
void setup(uint16_t back_color, bool add);
void create_buffers();       // Allocate _sprite_back and _sprite_content
void clear_buffers();        // Release sprites
void clear_content();        // Fill _sprite_content with TFT_MAGENTA

void refresh(bool forced, bool force_children);
bool position_children(bool force_children);  // Layout + redraw children
bool redraw(uint8_t fade_amount, int8_t tab_group);

void show_background_jpg(const void *jpg, int size, bool fade);
```

### Screen Drag Transitions

Screens support swipe-to-navigate via touch dragging. The drag system handles the visual transition between linked screens.

#### Drag State (`ui_screen` members)
```cpp
bool is_dragging;                          // Currently in a drag
bool is_drag_blended;                      // Content blend done for this drag
DRAGGABLE drag_axis;                       // DRAG_HORIZONTAL or DRAG_VERTICAL
int16_t drag_x, drag_y;                    // Current drag displacement
int16_t cached_drag_x, cached_drag_y;      // Previous frame displacement
int16_t last_delta_x, last_delta_y;        // Per-frame movement delta (flick velocity)
ui_screen *drag_neighbours[2];             // [0]=LEFT/UP, [1]=RIGHT/DOWN
umgfx::UM_GFX_Canvas _sprite_drag;        // Temporary 480x480 compositing buffer
```

#### Drag Lifecycle

**1. Drag Start** (`process_touch` when `!is_dragging`):
- Sets `is_dragging = true`, `is_drag_blended = false`
- Determines axis from touch event type (SCREEN_DRAG_H or SCREEN_DRAG_V)
- Populates `drag_neighbours[]` from `navigation[]`
- Calls `setup_draggable_neighbour(true)` on neighbours (allocates their sprite buffers)
- Creates `_sprite_drag` (480x480 temporary buffer)

**2. Drag Movement** (subsequent SCREEN_DRAG_H/V events):
- Updates `drag_x`/`drag_y` from touch delta
- Calls `draw_draggable()` when position changed

**3. Drag Release** (`process_touch` with `TOUCH_UNKNOWN` while `is_dragging`):
- If `abs(last_delta_x) > 25` or `abs(last_delta_y) > 25`: flick detected → `finish_drag()`
- Otherwise: `cancel_drag()` snaps back to origin
- Resets `is_dragging = false`, `is_drag_blended = false`, `drag_axis = DRAG_NONE`

#### `draw_draggable()` - Per-Frame Drag Rendering

```
draw_draggable()
    │
    ├─ if (!is_drag_blended):
    │      blendSprite(_sprite_content, _sprite_back, _sprite_content, 32, TFT_MAGENTA)
    │      is_drag_blended = true
    │      (This pre-composites content over background ONCE per drag)
    │
    ├─ drawSprite(drag_x, drag_y, &_sprite_content) into _sprite_drag
    │
    ├─ Draw neighbour screen in exposed gap:
    │      drag_x > 0 → draw drag_neighbours[1] (RIGHT) at drag_x - 480
    │      drag_x < 0 → draw drag_neighbours[0] (LEFT) at drag_x + 480
    │      (same pattern for drag_y with UP/DOWN)
    │
    └─ blendSprite(_sprite_drag, lcd, lcd, 32) → visible on screen
```

**`is_drag_blended` optimization:** The expensive content-over-background blend (`blendSprite` with 480x480 pixels) only runs once at the start of a drag, not on every pixel of movement. This is controlled by `is_drag_blended`:
- Set to `false` when a new drag starts (line 483)
- Set to `true` after the first blend in `draw_draggable()` (line 784)
- Set to `false` when drag ends (line 557)

**Note:** Because the blend writes into `_sprite_content` itself (source and destination), the content sprite is modified during drag. Normal widget rendering resumes after the drag ends and `_sprite_content` gets rebuilt by widget redraws.

#### `finish_drag(direction, dx, dy)`

Animates the screen sliding to completion:
- Calculates target position (e.g., LEFT → to_x = -480)
- Loops: `drag_x = round(drag_x + (to_x - drag_x) / 1.3)` with `draw_draggable()` each step
- Calls `about_to_close_screen()` on old screen
- Switches current screen to `navigation[(int)direction]`
- Calls `about_to_show_screen()` and `refresh(true)` on new screen

#### `cancel_drag()`

Animates the screen snapping back to origin:
- Loops: `drag_x = round(drag_x / 3.0)` with `draw_draggable()` each step until 0
- Releases `_sprite_drag` buffer
- Calls `clean_neighbour_sprites()` to free neighbour buffers

#### `draw_draggable_neighbour(sprite, dx, dy)`

Called on neighbour screens during drag to draw their composited content into the drag sprite at the given offset. Blends the neighbour's `_sprite_content` over its `_sprite_back` and draws the result.

#### Screen `refresh()` During Drag

When `is_dragging` is true, `refresh()` returns immediately — no child updates or screen compositing. All rendering during drag is handled by `draw_draggable()`.

#### Known Issue: Drag Direction Overshoot

When dragging in one direction and then reversing the flick past center, the screen can overshoot and transition to the opposite neighbour. The `finish_drag` direction is determined by `last_delta_x`/`last_delta_y` (flick velocity) without checking whether the displacement matches the original drag direction.

### Rendering Flow (refresh → redraw)

1. `refresh()` is called from main loop when `should_refresh()` returns true
2. If `is_dragging` is true, `refresh()` returns immediately (drag rendering is separate)
3. `refresh()` calls `position_children()` which:
   - If screen has `ui_tab_group`: draws tab bar, then iterates `tab_group_children[current_tab]`
   - Otherwise: iterates `ui_children`
   - Calls `reposition()` on each child (for grid layout)
   - Calls `child->redraw(32)` if dirty or refresh due
4. If any child was dirty OR animations are active OR forced, `refresh()` calls `redraw(32)`
5. `redraw()` blends `_sprite_content` over `_sprite_back` to `squixl.lcd`:
   ```cpp
   squixl.lcd.blendSprite(&_sprite_content, &_sprite_back, &squixl.lcd, 32, TFT_MAGENTA);
   ```

**Animation impact on refresh rate:**
- Default `refresh_interval` is 20ms (~50 checks/sec)
- When `animation_manager.active_animations() > 0`, `refresh_interval` drops to 1ms and `redraw(32)` runs every cycle — this is expensive (full 480x480 blend each time)
- When animations complete, `refresh_interval` returns to 20ms
- Backlight animations (500ms on touch after dimming) trigger this fast refresh path

---

## ui_control (Interactive Control Base)

Base class for buttons, toggles, sliders, textboxes.

### Creation
```cpp
void create(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *title);
void create_on_grid(uint8_t span_c, uint8_t span_r, const char *title);
```

Grid layout: 6 columns × 6 rows, 80px column width, 80px row height, 10px padding.

### Settings Binding
Controls bind to `SettingsOption*` objects for automatic value sync:
```cpp
SettingsOptionBase *setting_option;
void set_options_data(SettingsOptionBase *sett);
```

### Sprite Management
```cpp
void clear_sprites();  // Releases _sprite_content
```

---

## ui_control_tabgroup

Manages tabbed UI within a screen.

### Setup
```cpp
ui_control_tabgroup *tabs = new ui_control_tabgroup();
tabs->create(0, 0, 480, 40);
tabs->set_tabs({"Tab1", "Tab2", "Tab3"});
screen->set_page_tabgroup(tabs);
```

### Adding Controls to Tabs
```cpp
// Add control to tab index 0
tabs->add_child_ui(some_control, 0);

// Add control to tab index 1
tabs->add_child_ui(other_control, 1);
```

Children are stored in `ui_element::tab_group_children[tab_index]`.

### Tab Switching (`process_touch`)
When user taps a different tab:
1. `clear_tabbed_children()` - calls `clear_sprites()` on current tab's children
2. `current_tab = new_tab`
3. `clear_content()` - fills screen's `_sprite_content` with TFT_MAGENTA
4. `refresh(true)` - forces full redraw

---

## ui_dialogbox

Modal dialog for user confirmations and messages. Global instance: `dialogbox`

### Usage
```cpp
// Simple message with OK button
dialogbox.set_button_ok("OK", []() { /* callback */ });
dialogbox.show("Title", "Message text", width, height);
dialogbox.draw();
squixl.current_screen()->redraw(32);

// Confirmation with OK and Cancel
dialogbox.set_button_ok("Yes", []() { /* on confirm */ });
dialogbox.set_button_cancel("No", []() { /* on cancel */ });
dialogbox.show("Confirm", "Are you sure?", 280, 140);

// Close programmatically
dialogbox.close();
```

---

## ui_keyboard

On-screen keyboard for text input. Global instance: `keyboard`

### Usage
```cpp
// Show keyboard for a textbox
keyboard.show(textbox_control, "Initial text");

// Keyboard handles input and updates the textbox
// Closes when user taps outside or presses enter
```

---

## Widgets

Widgets are specialized `ui_element` subclasses for displaying data:

| Widget | Description |
|--------|-------------|
| `widgetTime` | Clock display with date |
| `widgetBattery` | Battery level and WiFi status icons |
| `widgetOpenWeather` | Weather data with icons |
| `widgetRSSFeeds` | RSS headlines with auto-scroll |
| `widgetJokes` | Random jokes from API |
| `widgetBME280` | Environmental sensor (temp/humidity/pressure) |
| `widgetMQTTSensors` | MQTT sensor data display |
| `widgetWifiManager` | WiFi connection status |
| `widgetFPS` | Loop iterations/sec performance counter |

Each widget:
1. Creates its own `_sprite_content` in `redraw()`
2. Draws its content to `_sprite_content`
3. Draws `_sprite_content` to parent's `_sprite_content`:
   ```cpp
   get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_content, 1.0f, -1);
   ```

### widgetFPS - Performance Counter

`widgetFPS` (`src/ui/widgets/widget_fps.h`, `src/ui/widgets/widget_fps.cpp`) measures main loop iterations per second. This is a performance diagnostic tool for identifying bottlenecks.

**How it works:**
- `tick()` is called at the very top of `loop()` in `main.cpp` on every iteration
- First call initializes `last_fps_time` and returns (avoids boot accumulation)
- Subsequent calls increment `loop_count`
- Every 1000ms, `fps = loop_count`, counter resets, `is_dirty = true`
- `redraw()` (called via `refresh_interval = 1000`) displays the stored `fps` value

**What the number means:**
- ~92K at 240MHz idle — normal baseline
- ~35K at 80MHz idle — CPU frequency scaled down by backlight dimmer after inactivity
- Drops during touch/drag — touch processing and screen compositing consume loop time
- The number reflects how much CPU headroom the application has

**Creation in `main.cpp`:**
```cpp
widget_fps = new widgetFPS();
widget_fps->create(10, 68, TFT_WHITE);  // Below battery widget (y=0, h=64, +4px gap)
widget_fps->set_refresh_interval(1000);
screen_main->add_child_ui(widget_fps);
```

**Important:** `tick()` must be the first call in `loop()` before any early returns, to count all iterations including those gated by `switching_screens` or other conditions.

---

## Constructing a Screen with UI

Example from `main.cpp`:

```cpp
// 1. Create screen (allocates in PSRAM via overloaded new)
screen_settings = new ui_screen();
screen_settings->setup(back_color, add_to_list);

// 2. Create tab group
settings_tab_group = new ui_control_tabgroup();
settings_tab_group->create(0, 0, 480, 40);
settings_tab_group->set_tabs({"General", "Location", "WiFi"});
screen_settings->set_page_tabgroup(settings_tab_group);

// 3. Add controls to tabs
slider_volume = new ui_control_slider();
slider_volume->create_on_grid(6, 1);  // 6 columns wide, 1 row tall
slider_volume->set_options_data(&settings.setting_volume);
settings_tab_group->add_child_ui(slider_volume, 0);  // Add to tab 0

// 4. Add non-tabbed children directly to screen
label_version.create(240, 460, "v1.0", TFT_GREY);
screen_settings->add_child_ui(&label_version);  // No tab group

// 5. Link screens for navigation
screen_main->set_navigation(Directions::DOWN, screen_settings, true);
```

---

## Rendering Pipeline Summary

```
Main Loop
    │
    ▼
screen->should_refresh()? ──yes──► screen->refresh()
                                        │
                                        ▼
                                  position_children()
                                        │
                                        ├─► tabgroup->redraw() [draws tab bar]
                                        │
                                        ├─► for each child in current tab:
                                        │       child->reposition()
                                        │       child->redraw(32)
                                        │           └─► draws to _sprite_content
                                        │           └─► draws _sprite_content to parent's _sprite_content
                                        │
                                        ▼
                                  screen->redraw(32)
                                        │
                                        ▼
                                  blendSprite(_sprite_content, _sprite_back, lcd, 32, TFT_MAGENTA)
                                        │
                                        ▼
                                  Content visible on LCD
```

---

## ui_scrollarea

Scrollable container for lists and content that exceeds screen bounds.

### Variants
- **ui_scrollarea** - Base scrollable container
- **ui_scrollarea_wifimanager** - WiFi network scan results (tap to select SSID)
- **ui_scrollarea_mqtt** - MQTT message notifications

### Key Features
- Touch drag to scroll vertically
- Momentum scrolling with deceleration
- Content clipping to viewport
- Override `render_content()` to draw custom content
- Override `external_content_dirty()` to trigger redraws from external data changes

---

## Memory Notes

- All `ui_screen`, `ui_control_*`, `widget_*` use overloaded `operator new` to allocate in PSRAM
- Sprites (`UM_GFX_Canvas`) allocate their buffers in PSRAM via `create()`
- `create(w, h)` leaves buffer uninitialized; `create(w, h, color)` fills with color
- Call `release()` or `clear_sprites()` to free sprite buffers when not needed
- `TFT_MAGENTA` (0xF81F) is the transparency key - pixels with this color are not drawn during blend

---

## Common Patterns

### Showing a Dialog
```cpp
dialogbox.set_button_ok("OK", []() { /* action */ });
dialogbox.show("Title", "Message", 280, 140);
dialogbox.draw();
squixl.current_screen()->redraw(32);
```

### Creating a Widget
```cpp
widget_time.create(x, y, width, height, TFT_BLACK, transparency, blur, "Title");
screen->add_child_ui(&widget_time);
```

### Forcing a Screen Refresh
```cpp
squixl.current_screen()->set_dirty(true);
squixl.current_screen()->refresh(true);
```
