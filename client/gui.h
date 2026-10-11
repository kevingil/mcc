#ifndef GUI_H
#define GUI_H

#include "voxel_types.h"

// In-game GUI drawn in GUI pixels. GuiBegin() scales by GuiScale(), so one
// GUI pixel covers GuiScale() screen pixels, the same way the menus scale.
// The sprites are painted in code at startup; none come from image files.

typedef enum {
    GUI_SPRITE_HOTBAR = 0,
    GUI_SPRITE_HOTBAR_SELECTION,
    GUI_SPRITE_HOTBAR_OFFHAND,
    GUI_SPRITE_CROSSHAIR,
    GUI_SPRITE_ATTACK_BACKGROUND,
    GUI_SPRITE_ATTACK_PROGRESS,
    GUI_SPRITE_HEART_CONTAINER,
    GUI_SPRITE_HEART_FULL,
    GUI_SPRITE_FOOD_EMPTY,
    GUI_SPRITE_FOOD_FULL,
    GUI_SPRITE_AIR,
    GUI_SPRITE_AIR_BURSTING,
    GUI_SPRITE_XP_BACKGROUND,
    GUI_SPRITE_SLOT,
    GUI_SPRITE_SLOT_LARGE,
    GUI_SPRITE_SCROLLER,
    GUI_SPRITE_SCROLLER_DISABLED,
    GUI_SPRITE_TAB_TOP,
    GUI_SPRITE_TAB_TOP_SELECTED_FIRST,
    GUI_SPRITE_TAB_TOP_SELECTED,
    GUI_SPRITE_TAB_TOP_SELECTED_LAST,
    GUI_SPRITE_TAB_BOTTOM,
    GUI_SPRITE_TAB_BOTTOM_SELECTED_FIRST,
    GUI_SPRITE_TAB_BOTTOM_SELECTED,
    GUI_SPRITE_TAB_BOTTOM_SELECTED_LAST,
    GUI_SPRITE_RECIPE_BUTTON,
    GUI_SPRITE_RECIPE_BUTTON_HIGHLIGHTED,
    GUI_SPRITE_EMPTY_HELMET,
    GUI_SPRITE_EMPTY_CHESTPLATE,
    GUI_SPRITE_EMPTY_LEGGINGS,
    GUI_SPRITE_EMPTY_BOOTS,
    GUI_SPRITE_EMPTY_SHIELD,
    GUI_SPRITE_DESTROY_SLOT,
    GUI_SPRITE_BOOK_TAB,
    GUI_SPRITE_BOOK_TAB_SELECTED,
    GUI_SPRITE_BOOK_SLOT_CRAFTABLE,
    GUI_SPRITE_BOOK_SLOT_UNCRAFTABLE,
    GUI_SPRITE_BOOK_SLOT_MANY_CRAFTABLE,
    GUI_SPRITE_BOOK_SLOT_MANY_UNCRAFTABLE,
    GUI_SPRITE_BOOK_OVERLAY_CRAFTABLE,
    GUI_SPRITE_BOOK_OVERLAY_CRAFTABLE_HIGHLIGHTED,
    GUI_SPRITE_BOOK_OVERLAY_UNCRAFTABLE,
    GUI_SPRITE_BOOK_OVERLAY_UNCRAFTABLE_HIGHLIGHTED,
    GUI_SPRITE_BOOK_FILTER_ALL,
    GUI_SPRITE_BOOK_FILTER_ALL_HIGHLIGHTED,
    GUI_SPRITE_BOOK_FILTER_CRAFTABLE,
    GUI_SPRITE_BOOK_FILTER_CRAFTABLE_HIGHLIGHTED,
    GUI_SPRITE_BOOK_PAGE_FORWARD,
    GUI_SPRITE_BOOK_PAGE_FORWARD_HIGHLIGHTED,
    GUI_SPRITE_BOOK_PAGE_BACKWARD,
    GUI_SPRITE_BOOK_PAGE_BACKWARD_HIGHLIGHTED,
    GUI_SPRITE_COUNT
} GuiSprite;

void GuiUnload(void);

int GuiScale(void);
int GuiWidth(void);
int GuiHeight(void);
Vector2 GuiMouse(void);

void GuiBegin(void);
void GuiEnd(void);
void GuiScissor(int x, int y, int w, int h);
void GuiScissorEnd(void);

int GuiSpriteWidth(GuiSprite sprite);
int GuiSpriteHeight(GuiSprite sprite);
void GuiDrawSprite(GuiSprite sprite, int x, int y);
void GuiDrawSpriteRegion(GuiSprite sprite, int sx, int sy, int w, int h, int x, int y);
void GuiDrawSpriteTinted(GuiSprite sprite, int x, int y, Color tint);

// Fills take corner coordinates: x0,y0 inclusive, x1,y1 exclusive.
void GuiFill(int x0, int y0, int x1, int y1, Color color);
void GuiFillGradient(int x0, int y0, int x1, int y1, Color top, Color bottom);
void GuiDrawPanel(int x, int y, int w, int h);
void GuiDrawInset(int x, int y, int w, int h, Color fill);

int GuiTextWidth(const char *text);
Color GuiShadowColor(Color color);
void GuiDrawText(const char *text, int x, int y, Color color, bool shadow);
void GuiDrawTextItalic(const char *text, int x, int y, Color color, bool shadow);

bool GuiIsFlatItem(BlockType block);
int GuiMaxStack(BlockType block);
void GuiDrawItem(BlockType block, int x, int y);
void GuiDrawItemScaled(BlockType block, float x, float y, float scale);
void GuiDrawItemTexture(const char *textureName, int x, int y);
// Fills color over the pixels the item covers and nowhere else in its 16x16 box.
void GuiDrawItemSilhouette(BlockType block, int x, int y, Color color);
void GuiDrawItemCount(int count, int x, int y);
void GuiDrawItemCountText(const char *text, int x, int y, Color color);
void GuiDrawItemStack(BlockType block, int count, int x, int y);
void GuiDrawSlotHighlight(int x, int y);
void GuiDrawTooltip(const char **lines, const Color *colors, int lineCount, int mouseX, int mouseY);
// italic may be NULL. A NULL colors array draws every line white.
void GuiDrawTooltipEx(const char **lines, const Color *colors, const bool *italic, int lineCount, int mouseX, int mouseY);

#endif // GUI_H
