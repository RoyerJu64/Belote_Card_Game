#include "cardgame/client/Ui.hpp"

namespace cardgame::client::ui {

void drawCentered(const char* text, float cx, float y, int size, Color color) {
    const int width = MeasureText(text, size);
    DrawText(text, static_cast<int>(cx) - width / 2, static_cast<int>(y), size, color);
}

bool button(Rectangle bounds, const char* label, bool enabled) {
    const Vector2 mouse = GetMousePosition();
    const bool hover = enabled && CheckCollisionPointRec(mouse, bounds);
    const Color fill = !enabled ? theme::kButtonDisabled
                                : (hover ? theme::kButtonHover : theme::kButton);

    DrawRectangleRounded(bounds, 0.25f, 8, fill);
    const int fontSize = 20;
    const int width = MeasureText(label, fontSize);
    DrawText(label, static_cast<int>(bounds.x + (bounds.width - static_cast<float>(width)) / 2),
             static_cast<int>(bounds.y + (bounds.height - static_cast<float>(fontSize)) / 2),
             fontSize, enabled ? theme::kText : theme::kTextDim);

    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void updateTextField(TextField& field, Rectangle bounds) {
    const Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        field.focused = CheckCollisionPointRec(mouse, bounds);
    }
    if (!field.focused) {
        return;
    }

    for (int c = GetCharPressed(); c > 0; c = GetCharPressed()) {
        if (c >= 32 && c < 127 && field.text.size() < field.maxLength) {
            field.text.push_back(static_cast<char>(c));
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !field.text.empty()) {
        field.text.pop_back();
    }
}

void drawTextField(const TextField& field, Rectangle bounds, const char* placeholder) {
    DrawRectangleRounded(bounds, 0.2f, 6, theme::kPanel);
    DrawRectangleLinesEx(bounds, 2.0f, field.focused ? theme::kAccent : theme::kTextDim);

    const bool empty = field.text.empty();
    const char* shown = empty ? placeholder : field.text.c_str();
    const Color color = empty ? theme::kTextDim : theme::kText;
    DrawText(shown, static_cast<int>(bounds.x) + 10,
             static_cast<int>(bounds.y + (bounds.height - 20) / 2), 20, color);

    if (field.focused) {
        const int caretX = static_cast<int>(bounds.x) + 10 +
                           (empty ? 0 : MeasureText(field.text.c_str(), 20));
        DrawRectangle(caretX + 2, static_cast<int>(bounds.y) + 8, 2,
                      static_cast<int>(bounds.height) - 16, theme::kAccent);
    }
}

} // namespace cardgame::client::ui
