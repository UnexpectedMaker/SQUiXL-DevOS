# Firmware Upgrade Guide: V1 to V2

This guide covers migrating custom code from the original V1 firmware (using BB_LCD_SPI) to V2 (using UM_GFX). If you've forked the original firmware and added custom widgets, screens, or functionality, follow this guide to update your code.

---

## Overview of Changes

| Component | V1 (BB_LCD_SPI) | V2 (UM_GFX) |
|-----------|-----------------|-------------|
| Graphics library | BB_SPI_LCD | UM_GFX |
| Sprite class | BB_SPI_LCD | umgfx::UM_GFX_Canvas |
| Display class | BB_SPI_LCD | umgfx::UM_GFX |
| Namespace | None (global) | `umgfx::` |
| Pixel format | Little-endian RGB565 | Big-endian RGB565 |

---

## Sprite/Canvas Changes

### Creating Sprites

**V1 (BB_LCD_SPI):**
```cpp
BB_SPI_LCD sprite;
sprite.createVirtual(width, height, NULL);  // Uninitialized buffer
sprite.createVirtual(width, height, buffer); // External buffer
```

**V2 (UM_GFX):**
```cpp
umgfx::UM_GFX_Canvas sprite;
sprite.create(width, height);              // Uninitialized buffer in PSRAM
sprite.create(width, height, TFT_BLACK);   // Fill with color
sprite.createWithBuffer(width, height, buffer); // External buffer
```

**Key differences:**
- `createVirtual()` → `create()` or `createWithBuffer()`
- The third parameter is now a **fill color** (`int fill_color = -1`), not a buffer pointer
- Use `createWithBuffer()` when you need to use an external buffer (rare)
- `-1` means no fill (uninitialized), any other value fills the sprite

### Releasing Sprites

**V1:**
```cpp
sprite.freeVirtual();
```

**V2:**
```cpp
sprite.release();
```

---

## Blend Functions

**V1:**
```cpp
// Blend same-size sprites
UM_GFX_Canvas::blendSprite(&foreground, &background, &dest, alpha, transparent);
```

**V2:**
```cpp
// Blend same-size sprites (same signature)
UM_GFX_Canvas::blendSprite(&foreground, &background, &dest, alpha, transparent);

// NEW: Blend smaller sprite at position (no intermediate buffer needed)
UM_GFX_Canvas::blendSpriteAt(&foreground, &dest, x, y, alpha, transparent);
```

**New `blendSpriteAt()` function:**
- Blends a smaller sprite directly into a destination at a specific position
- Eliminates the need for intermediate buffers
- Uses SIMD-optimized assembly for performance
- Alpha range: 0-32 (same as always)

---

## Pixel Format

V2 uses **big-endian RGB565** to match the RGB panel's native format. This eliminates byte-swapping overhead.

**If you're working with raw pixel data:**
```cpp
// V1: Little-endian - low byte first
uint16_t pixel = (green_high << 8) | (red << 3) | (blue >> 3);

// V2: Big-endian - high byte first
// The library handles this internally, but if you need manual conversion:
uint16_t swapped = __builtin_bswap16(pixel);
```

For most use cases, the library handles byte order automatically. Only worry about this if you're directly manipulating pixel buffers.

---

## Namespace Changes

V2 uses the `umgfx` namespace.

**V1:**
```cpp
BB_SPI_LCD sprite;
```

**V2:**
```cpp
umgfx::UM_GFX_Canvas sprite;

// Or at file scope:
using namespace umgfx;
UM_GFX_Canvas sprite;
```

---

## UI Element Sprites

If you've created custom widgets or UI elements, update sprite member types:

**V1:**
```cpp
class MyWidget : public ui_element {
    BB_SPI_LCD _sprite_content;
    BB_SPI_LCD _sprite_clean;
};
```

**V2:**
```cpp
class MyWidget : public ui_element {
    umgfx::UM_GFX_Canvas _sprite_content;
    umgfx::UM_GFX_Canvas _sprite_clean;
};
```

---

## Common Drawing Functions

Most drawing functions have the same signatures:

| Function | Notes |
|----------|-------|
| `fillRect(x, y, w, h, color)` | Same |
| `drawRect(x, y, w, h, color)` | Same |
| `fillRoundRect(x, y, w, h, r, color)` | Same |
| `drawRoundRect(x, y, w, h, r, color)` | Same |
| `fillCircle(x, y, r, color)` | Same |
| `drawCircle(x, y, r, color)` | Same |
| `drawLine(x0, y0, x1, y1, color)` | Same |
| `fillScreen(color)` | Same |
| `drawPixel(x, y, color)` | Same |
| `readPixel(x, y)` | Same |
| `setFreeFont(font)` | Same |
| `setCursor(x, y)` | Same |
| `setTextColor(fg, bg)` | Same |
| `print()` / `printf()` | Same (inherits from Print) |

---

## Text Rendering

Text rendering API is unchanged:

```cpp
sprite.setFreeFont(&MyFont);
sprite.setTextColor(TFT_WHITE, TFT_BLACK);
sprite.setCursor(10, 20);
sprite.print("Hello World");
sprite.printf("Value: %d", value);
```

---

## Image Loading

**PNG and JPEG loading:**
```cpp
PNGDisplay png;
png.loadPNG(&sprite, x, y, data, size, bgColor);

JPEGDisplay jpeg;
jpeg.loadJPEG(&sprite, x, y, data, size, options);
jpeg.loadJPEG_LFS(&sprite, x, y, "/path/file.jpg", options);
```

The API is similar but now uses `UM_GFX_Canvas` instead of `BB_SPI_LCD`.

---

## Migration Checklist

1. **Update sprite types:**
   - [ ] `BB_SPI_LCD` → `umgfx::UM_GFX_Canvas`

2. **Update sprite creation:**
   - [ ] `createVirtual(w, h, NULL)` → `create(w, h)`
   - [ ] `createVirtual(w, h, buf)` → `createWithBuffer(w, h, buf)`
   - [ ] `freeVirtual()` → `release()`

3. **Update blend calls:**
   - [ ] Consider using new `blendSpriteAt()` for positioned blending

4. **Add namespace:**
   - [ ] Add `umgfx::` prefix or `using namespace umgfx;`

5. **Test transparency:**
   - [ ] Verify `TFT_MAGENTA` (0xF81F) still works as transparency key

---

## Example: Widget Migration

**V1 Widget:**
```cpp
class MyWidget : public ui_element {
public:
    bool redraw(uint8_t fade_amount, int8_t tab_group) override {
        if (!is_setup) {
            _sprite_content.createVirtual(_w, _h, NULL);
            _sprite_content.fillScreen(TFT_BLACK);
            is_setup = true;
        }

        _sprite_content.setFreeFont(&FreeSans12pt7b);
        _sprite_content.setTextColor(TFT_WHITE, TFT_BLACK);
        _sprite_content.setCursor(10, 30);
        _sprite_content.print("Hello");

        // Draw to parent
        UM_GFX_Canvas::blendSprite(&_sprite_content, &_sprite_clean, &_sprite_mixed, 32, TFT_MAGENTA);
        get_ui_parent()->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1);

        return true;
    }

private:
    BB_SPI_LCD _sprite_content;
    BB_SPI_LCD _sprite_clean;
    BB_SPI_LCD _sprite_mixed;
    bool is_setup = false;
};
```

**V2 Widget:**
```cpp
class MyWidget : public ui_element {
public:
    bool redraw(uint8_t fade_amount, int8_t tab_group) override {
        if (!is_setup) {
            _sprite_content.create(_w, _h, TFT_BLACK);  // Create sprite with BLACK fill
            is_setup = true;
        }

        _sprite_content.setFreeFont(&FreeSans12pt7b);
        _sprite_content.setTextColor(TFT_WHITE, TFT_BLACK);
        _sprite_content.setCursor(10, 30);
        _sprite_content.print("Hello");

        // Use blendSpriteAt for direct positioned blend using SIMD (requires width/height are divisivle by 8 (NEW)
        UM_GFX_Canvas::blendSpriteAt(&_sprite_content, &get_ui_parent()->_sprite_content, _x, _y, 32, TFT_MAGENTA);

        return true;
    }

private:
    umgfx::UM_GFX_Canvas _sprite_content;
    umgfx::UM_GFX_Canvas _sprite_clean;
    umgfx::UM_GFX_Canvas _sprite_mixed;
    bool is_setup = false;
};
```

---

## Troubleshooting

**Colors look wrong:**
- Check that you're not manually byte-swapping pixels (V2 handles this)

**Transparency not working:**
- Ensure you're using `TFT_MAGENTA` (0xF81F) as the transparency color
- Pass `-1` to disable transparency, or the specific color to enable it

**Sprites appear corrupted:**
- Make sure you're calling `create()` before drawing
- Check that sprite dimensions are correct

**Compilation errors about BB_SPI_LCD:**
- Replace with `umgfx::UM_GFX_Canvas`
- Add `using namespace umgfx;` if preferred
