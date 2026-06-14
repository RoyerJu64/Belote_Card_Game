#pragma once

#include <cstddef>
#include <string>

#include "raylib.h"

namespace cardgame::client::ui {

// A small immediate-mode toolkit — enough for a few buttons and text fields
// without pulling in a UI library.

namespace theme {
inline constexpr Color kTable{16, 92, 56, 255};
inline constexpr Color kPanel{24, 28, 36, 235};
inline constexpr Color kButton{44, 96, 140, 255};
inline constexpr Color kButtonHover{64, 128, 184, 255};
inline constexpr Color kButtonDisabled{70, 74, 82, 255};
inline constexpr Color kAccent{222, 184, 64, 255};
inline constexpr Color kText{236, 238, 242, 255};
inline constexpr Color kTextDim{150, 156, 168, 255};
} // namespace theme

/// Centered text helper.
void drawCentered(const char* text, float cx, float y, int size, Color color);

/// Returns true on the frame the button is clicked. Disabled buttons render
/// greyed and never fire.
[[nodiscard]] bool button(Rectangle bounds, const char* label, bool enabled = true);

/// A single-line editable text field. Click to focus; types printable ASCII.
struct TextField {
    std::string text;
    std::size_t maxLength{24};
    bool focused{false};
};

void updateTextField(TextField& field, Rectangle bounds);
void drawTextField(const TextField& field, Rectangle bounds, const char* placeholder);

} // namespace cardgame::client::ui
