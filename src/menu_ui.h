#ifndef MENU_UI_H
#define MENU_UI_H

#include "raylib.h"

#define OPENCRAFT_VERSION "0.1.0"

typedef enum {
    UNAVAILABLE_MULTIPLAYER = 0,
    UNAVAILABLE_REALMS = 1
} UnavailableMenu;

typedef struct {
    Rectangle bounds;
    const char *label;
    bool enabled;
    bool hovered;
    bool clicked;
} MenuButton;

void InitMenuUi(void);
void UnloadMenuUi(void);
void UpdateMenuUi(void);

int MenuScale(void);
Font GetMenuFont(void);

void DrawPanoramaBackground(void);
void DrawDirtBackground(void);
void DrawMenuText(int x, int y, int fontSize, const char *text, Color color);
void DrawMenuTextCentered(int centerX, int y, int fontSize, const char *text, Color color);
int MenuTextWidth(const char *text, int fontSize);
void DrawMenuTextField(Rectangle bounds, const char *text, bool focused, int frames);

void UpdateMenuButton(MenuButton *button);
void DrawStoneButton(const MenuButton *button, bool emphasized);
void DrawLanguageButton(const MenuButton *button, bool emphasized);
void DrawAccessibilityButton(const MenuButton *button, bool emphasized);

// topY is the top of the front face. The extrusion extends above it.
Rectangle DrawOpenCraftLogo(int centerX, int topY, int scale);

void SetUnavailableMenu(UnavailableMenu menu);
UnavailableMenu GetUnavailableMenu(void);

#endif // MENU_UI_H
