#ifndef TEXT_FIELD_H
#define TEXT_FIELD_H

#include "raylib.h"

#define TEXT_FIELD_CAPACITY 64

// A borderless one-line edit box in GUI pixels with the usual keys: arrows
// (Ctrl jumps words, Shift extends the selection), Home, End, Backspace,
// Delete, and Ctrl+A, C, V, X. The caller paints whatever sits behind it.
typedef struct {
    char value[TEXT_FIELD_CAPACITY];
    int maxLength;
    int width;
    int cursor;
    int highlight;
    int display;
    bool focused;
    double focusTime;
} TextField;

void TextFieldInit(TextField *field, int maxLength, int width);
void TextFieldSetValue(TextField *field, const char *text);
void TextFieldSetFocus(TextField *field, bool focused);
// Puts the cursor at the end with everything selected.
void TextFieldSelectAll(TextField *field);
bool TextFieldKeyPressed(TextField *field, int key);
bool TextFieldCharTyped(TextField *field, int codepoint);
// A press anywhere focuses the field when inside it and, if canLoseFocus,
// unfocuses it otherwise. A left press inside moves the cursor under the mouse.
bool TextFieldMouseClicked(TextField *field, int x, int y, int height, int mouseX, int mouseY, int button, bool canLoseFocus);
void TextFieldDraw(const TextField *field, int x, int y, Color color);

#endif // TEXT_FIELD_H
