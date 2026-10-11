#include "text_field.h"
#include "gui.h"

#include "rlgl.h"

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    #include <GLES2/gl2.h>
#else
    #include <GL/gl.h>
#endif

#include <string.h>

// Selection keys only extend the selection while Shift is down for the key
// being handled; Backspace and Delete ignore it.
static bool shiftHeld = false;

static bool ShiftDown(void)
{
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

static bool ControlDown(void)
{
#if defined(__APPLE__)
    return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
#else
    return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
#endif
}

static bool AltDown(void)
{
    return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
}

static int Length(const TextField *field)
{
    return (int)strlen(field->value);
}

static int ClampInt(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static int PrefixWidth(const char *text, int count)
{
    char buffer[TEXT_FIELD_CAPACITY] = { 0 };

    if (count <= 0) return 0;
    if (count > TEXT_FIELD_CAPACITY - 1) count = TEXT_FIELD_CAPACITY - 1;
    memcpy(buffer, text, (size_t)count);
    return GuiTextWidth(buffer);
}

// How many characters from the start of text fit in width GUI pixels.
static int FitForward(const char *text, int width)
{
    int length = (int)strlen(text);
    int count = 0;

    while ((count < length) && (PrefixWidth(text, count + 1) <= width)) count++;
    return count;
}

// How many characters from the end of text fit in width GUI pixels.
static int FitBackward(const char *text, int width)
{
    int length = (int)strlen(text);
    int count = 0;

    while ((count < length) && (GuiTextWidth(text + length - count - 1) <= width)) count++;
    return count;
}

static void SetHighlight(TextField *field, int position)
{
    int length = Length(field);
    int end = 0;

    field->highlight = ClampInt(position, 0, length);
    if (field->display > length) field->display = length;
    end = field->display + FitForward(field->value + field->display, field->width);
    if (field->highlight == field->display) field->display -= FitBackward(field->value, field->width);
    if (field->highlight > end) field->display += field->highlight - end;
    else if (field->highlight <= field->display) field->display -= field->display - field->highlight;
    field->display = ClampInt(field->display, 0, length);
}

static void MoveCursorTo(TextField *field, int position)
{
    field->cursor = ClampInt(position, 0, Length(field));
    if (!shiftHeld) SetHighlight(field, field->cursor);
}

static bool AllowedCharacter(int codepoint)
{
    return (codepoint >= 32) && (codepoint < 127);
}

static void InsertText(TextField *field, const char *text)
{
    char filtered[TEXT_FIELD_CAPACITY] = { 0 };
    char result[TEXT_FIELD_CAPACITY] = { 0 };
    int start = (field->cursor < field->highlight)? field->cursor : field->highlight;
    int end = (field->cursor < field->highlight)? field->highlight : field->cursor;
    int length = Length(field);
    int room = field->maxLength - length + (end - start);
    int count = 0;

    for (int i = 0; (text[i] != '\0') && (count < TEXT_FIELD_CAPACITY - 1); i++)
    {
        if (AllowedCharacter((unsigned char)text[i])) filtered[count++] = text[i];
    }
    if (count > room) count = (room > 0)? room : 0;
    memcpy(result, field->value, (size_t)start);
    memcpy(result + start, filtered, (size_t)count);
    memcpy(result + start + count, field->value + end, (size_t)(length - end));
    memcpy(field->value, result, sizeof(field->value));
    field->cursor = ClampInt(start + count, 0, Length(field));
    SetHighlight(field, field->cursor);
}

// Forward: past the next space and any spaces after it. Backward: over the
// spaces before the cursor, then to the start of that word.
static int WordPosition(const TextField *field, int count, int from)
{
    int length = Length(field);
    int position = from;
    int steps = (count < 0)? -count : count;

    for (int k = 0; k < steps; k++)
    {
        if (count > 0)
        {
            const char *space = strchr(field->value + position, ' ');

            if (space == NULL)
            {
                position = length;
            }
            else
            {
                position = (int)(space - field->value);
                while ((position < length) && (field->value[position] == ' ')) position++;
            }
        }
        else
        {
            while ((position > 0) && (field->value[position - 1] == ' ')) position--;
            while ((position > 0) && (field->value[position - 1] != ' ')) position--;
        }
    }
    return position;
}

static void DeleteChars(TextField *field, int count)
{
    int length = Length(field);
    int target = 0;
    int start = 0;
    int end = 0;

    if (length == 0) return;
    if (field->highlight != field->cursor)
    {
        InsertText(field, "");
        return;
    }
    target = ClampInt(field->cursor + count, 0, length);
    start = (target < field->cursor)? target : field->cursor;
    end = (target < field->cursor)? field->cursor : target;
    if (start == end) return;
    memmove(field->value + start, field->value + end, (size_t)(length - end + 1));
    MoveCursorTo(field, start);
}

static void DeleteText(TextField *field, int count)
{
    if (Length(field) == 0) return;
    if (!ControlDown())
    {
        DeleteChars(field, count);
        return;
    }
    if (field->highlight != field->cursor) InsertText(field, "");
    else DeleteChars(field, WordPosition(field, count, field->cursor) - field->cursor);
}

static void CopySelection(const TextField *field)
{
    char buffer[TEXT_FIELD_CAPACITY] = { 0 };
    int start = (field->cursor < field->highlight)? field->cursor : field->highlight;
    int end = (field->cursor < field->highlight)? field->highlight : field->cursor;

    memcpy(buffer, field->value + start, (size_t)(end - start));
    SetClipboardText(buffer);
}

void TextFieldInit(TextField *field, int maxLength, int width)
{
    *field = (TextField){ 0 };
    field->maxLength = (maxLength < TEXT_FIELD_CAPACITY - 1)? maxLength : TEXT_FIELD_CAPACITY - 1;
    field->width = width;
}

void TextFieldSetValue(TextField *field, const char *text)
{
    int length = (int)strlen(text);

    if (length > field->maxLength) length = field->maxLength;
    memset(field->value, 0, sizeof(field->value));
    memcpy(field->value, text, (size_t)length);
    shiftHeld = false;
    MoveCursorTo(field, length);
    SetHighlight(field, field->cursor);
}

void TextFieldSetFocus(TextField *field, bool focused)
{
    if (focused && !field->focused) field->focusTime = GetTime();
    field->focused = focused;
}

void TextFieldSelectAll(TextField *field)
{
    shiftHeld = false;
    MoveCursorTo(field, Length(field));
    SetHighlight(field, 0);
}

bool TextFieldKeyPressed(TextField *field, int key)
{
    bool control = ControlDown();
    bool shift = ShiftDown();

    if (!field->focused) return false;
    shiftHeld = shift;
    if (control && !shift && !AltDown())
    {
        if (key == KEY_A)
        {
            MoveCursorTo(field, Length(field));
            SetHighlight(field, 0);
            return true;
        }
        if (key == KEY_C)
        {
            CopySelection(field);
            return true;
        }
        if (key == KEY_V)
        {
            const char *clipboard = GetClipboardText();

            InsertText(field, (clipboard != NULL)? clipboard : "");
            return true;
        }
        if (key == KEY_X)
        {
            CopySelection(field);
            InsertText(field, "");
            return true;
        }
    }
    switch (key)
    {
        case KEY_BACKSPACE:
        case KEY_DELETE:
            shiftHeld = false;
            DeleteText(field, (key == KEY_BACKSPACE)? -1 : 1);
            shiftHeld = shift;
            return true;
        case KEY_RIGHT:
            MoveCursorTo(field, control? WordPosition(field, 1, field->cursor) : field->cursor + 1);
            return true;
        case KEY_LEFT:
            MoveCursorTo(field, control? WordPosition(field, -1, field->cursor) : field->cursor - 1);
            return true;
        case KEY_HOME:
            MoveCursorTo(field, 0);
            return true;
        case KEY_END:
            MoveCursorTo(field, Length(field));
            return true;
        default:
            return false;
    }
}

bool TextFieldCharTyped(TextField *field, int codepoint)
{
    char text[2] = { 0 };

    if (!field->focused || !AllowedCharacter(codepoint)) return false;
    text[0] = (char)codepoint;
    InsertText(field, text);
    return true;
}

bool TextFieldMouseClicked(TextField *field, int x, int y, int height, int mouseX, int mouseY, int button, bool canLoseFocus)
{
    bool inside = (mouseX >= x) && (mouseX < x + field->width) && (mouseY >= y) && (mouseY < y + height);
    char visible[TEXT_FIELD_CAPACITY] = { 0 };

    if (canLoseFocus) TextFieldSetFocus(field, inside);
    if (!field->focused || !inside || (button != 0)) return false;
    memcpy(visible, field->value + field->display, (size_t)FitForward(field->value + field->display, field->width));
    shiftHeld = ShiftDown();
    MoveCursorTo(field, FitForward(visible, mouseX - x) + field->display);
    return true;
}

// The selection inverts red and green under it and saturates blue, so white
// text turns blue on a pale blue band.
static void DrawHighlight(int x0, int y0, int x1, int y1, int fieldX, int width)
{
    int left = (x0 < x1)? x0 : x1;
    int right = (x0 < x1)? x1 : x0;
    int top = (y0 < y1)? y0 : y1;
    int bottom = (y0 < y1)? y1 : y0;

    if (left > fieldX + width) left = fieldX + width;
    if (right > fieldX + width) right = fieldX + width;
    rlDrawRenderBatchActive();
#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    GuiFill(left, top, right, bottom, (Color){ 0, 0, 255, 96 });
#else
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_OR_REVERSE);
    GuiFill(left, top, right, bottom, (Color){ 0, 0, 255, 255 });
    rlDrawRenderBatchActive();
    glDisable(GL_COLOR_LOGIC_OP);
#endif
}

void TextFieldDraw(const TextField *field, int x, int y, Color color)
{
    char visible[TEXT_FIELD_CAPACITY] = { 0 };
    char prefix[TEXT_FIELD_CAPACITY] = { 0 };
    int length = Length(field);
    int cursorRel = field->cursor - field->display;
    int highlightRel = field->highlight - field->display;
    int visibleLength = FitForward(field->value + field->display, field->width);
    int frame = (int)((GetTime() - field->focusTime)*20.0);
    bool cursorInView = (cursorRel >= 0) && (cursorRel <= visibleLength);
    bool blink = field->focused && ((frame/6)%2 == 0) && cursorInView;
    bool barCursor = (field->cursor < length) || (length >= field->maxLength);
    int textEnd = x;
    int cursorX = x;

    memcpy(visible, field->value + field->display, (size_t)visibleLength);
    if (highlightRel > visibleLength) highlightRel = visibleLength;
    if (visibleLength > 0)
    {
        memcpy(prefix, visible, (size_t)(cursorInView? cursorRel : visibleLength));
        GuiDrawText(prefix, x, y, color, true);
        textEnd = x + GuiTextWidth(prefix) + 1;
    }
    cursorX = textEnd;
    if (!cursorInView)
    {
        cursorX = (cursorRel > 0)? x + field->width : x;
    }
    else if (barCursor)
    {
        cursorX = textEnd - 1;
        textEnd--;
    }
    if ((visibleLength > 0) && cursorInView && (cursorRel < visibleLength)) GuiDrawText(visible + cursorRel, textEnd, y, color, true);
    if (blink)
    {
        if (barCursor) GuiFill(cursorX, y - 1, cursorX + 1, y + 10, (Color){ 208, 208, 208, 255 });
        else GuiDrawText("_", cursorX, y, color, true);
    }
    if (highlightRel != cursorRel) DrawHighlight(cursorX, y - 1, x + PrefixWidth(visible, highlightRel) - 1, y + 10, x, field->width);
}
