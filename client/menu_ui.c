#include "menu_ui.h"

#include "rlgl.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Module Variables
//----------------------------------------------------------------------------------
static bool menuReady = false;
static bool fontOwned = false;
static Font menuFont = { 0 };
static Texture2D panorama[6] = { 0 };
static Texture2D panoramaOverlay = { 0 };
static Texture2D dirtTexture = { 0 };
static Texture2D stoneTexture = { 0 };
static Texture2D widgetsTexture = { 0 };
static float panoramaTimer = 0.0f;
static UnavailableMenu unavailableMenu = UNAVAILABLE_MULTIPLAYER;

//----------------------------------------------------------------------------------
// Local Functions
//----------------------------------------------------------------------------------
static Texture2D LoadUiTexture(const char *path, int filter)
{
    Texture2D texture = LoadTexture(path);

    if (texture.id != 0) SetTextureFilter(texture, filter);
    return texture;
}

// The title panorama is the Good Vibes art. Shift its baked cyan water and
// yellow sand toward the same blues and beiges the world uses at load time.
static void ShiftPanoramaPixel(Color *pixel)
{
    float r = pixel->r/255.0f;
    float g = pixel->g/255.0f;
    float b = pixel->b/255.0f;
    float max = r;
    float min = r;
    float hue = 0.0f;
    float span = 0.0f;
    float sat = 0.0f;
    float nr = 0.0f;
    float ng = 0.0f;
    float nb = 0.0f;
    float hueDeg = 0.0f;
    int sector = 0;
    float f = 0.0f;
    float p = 0.0f;
    float q = 0.0f;
    float t = 0.0f;

    if (g > max) max = g;
    if (b > max) max = b;
    if (g < min) min = g;
    if (b < min) min = b;
    span = max - min;
    if ((max <= 0.0f) || (span <= 0.0001f)) return;
    sat = span/max;
    if (max == r) hue = (g - b)/span;
    else if (max == g) hue = 2.0f + (b - r)/span;
    else hue = 4.0f + (r - g)/span;
    if (hue < 0.0f) hue += 6.0f;
    hueDeg = hue*60.0f;

    if ((hueDeg >= 40.0f) && (hueDeg <= 68.0f) && (sat > 0.28f) && (max > 0.55f) && (b < ((g < r)? g : r) - 0.10f))
    {
        hue = 46.0f/60.0f;
        sat *= 0.42f;
        max = max*0.92f + 0.06f;
        if (max > 1.0f) max = 1.0f;
    }
    else if ((hueDeg >= 155.0f) && (hueDeg <= 205.0f) && (sat > 0.18f) && (max > 0.25f))
    {
        hue = 208.0f/60.0f;
        sat *= 0.95f;
        if (sat > 1.0f) sat = 1.0f;
    }
    else if ((hueDeg >= 68.0f) && (hueDeg <= 140.0f) && (sat > 0.20f) && (g > r) && (g > b + 0.04f))
    {
        hue = 92.0f/60.0f;
        if (sat < 0.35f) sat = 0.35f;
    }
    else return;

    sector = (int)hue;
    f = hue - (float)sector;
    p = max*(1.0f - sat);
    q = max*(1.0f - sat*f);
    t = max*(1.0f - sat*(1.0f - f));
    if (sector < 0) sector = 0;
    switch (sector%6)
    {
        case 0: nr = max; ng = t; nb = p; break;
        case 1: nr = q; ng = max; nb = p; break;
        case 2: nr = p; ng = max; nb = t; break;
        case 3: nr = p; ng = q; nb = max; break;
        case 4: nr = t; ng = p; nb = max; break;
        default: nr = max; ng = p; nb = q; break;
    }
    pixel->r = (unsigned char)(nr*255.0f);
    pixel->g = (unsigned char)(ng*255.0f);
    pixel->b = (unsigned char)(nb*255.0f);
}

static Texture2D LoadPanorama(const char *path)
{
    Image image = LoadImage(path);
    Texture2D texture = { 0 };
    Color *pixels = NULL;
    int count = 0;
    int i = 0;

    if (image.data == NULL) return texture;
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    pixels = (Color *)image.data;
    count = image.width*image.height;
    for (i = 0; i < count; i++) ShiftPanoramaPixel(&pixels[i]);
    texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id != 0) SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

static Texture2D LoadNeutralStone(void)
{
    Image image = LoadImage("resources/textures/block/stone.png");
    Texture2D texture = { 0 };

    if (image.data == NULL) return texture;

    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = (Color *)image.data;
    int count = image.width*image.height;

    for (int i = 0; i < count; i++)
    {
        int lum = (pixels[i].r*3 + pixels[i].g*4 + pixels[i].b)/8;
        pixels[i].r = (unsigned char)lum;
        pixels[i].g = (unsigned char)lum;
        pixels[i].b = (unsigned char)lum;
    }

    texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id != 0) SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

static void LoadMenuFont(void)
{
    Image image = LoadImage("resources/textures/font/ascii.png");

    menuFont = GetFontDefault();
    fontOwned = false;
    if (image.data == NULL) return;

    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = (Color *)image.data;
    int count = image.width*image.height;

    for (int i = 0; i < count; i++)
    {
        if (pixels[i].a > 0)
        {
            pixels[i].r = 255;
            pixels[i].g = 255;
            pixels[i].b = 255;
        }
    }

    menuFont = (Font){ 0 };
    menuFont.baseSize = 8;
    menuFont.glyphCount = 128;
    menuFont.glyphPadding = 0;
    menuFont.recs = (Rectangle *)MemAlloc((unsigned int)(128*sizeof(Rectangle)));
    menuFont.glyphs = (GlyphInfo *)MemAlloc((unsigned int)(128*sizeof(GlyphInfo)));

    for (int i = 0; i < 128; i++)
    {
        int col = i%16;
        int row = i/16;
        int maxX = 0;
        bool any = false;

        menuFont.glyphs[i] = (GlyphInfo){ 0 };
        menuFont.recs[i] = (Rectangle){ (float)(col*8), (float)(row*8), 8.0f, 8.0f };
        menuFont.glyphs[i].value = i;
        menuFont.glyphs[i].offsetX = 0;
        menuFont.glyphs[i].offsetY = 0;

        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                Color pixel = pixels[(row*8 + y)*image.width + col*8 + x];
                if (pixel.a > 0)
                {
                    any = true;
                    if (x + 1 > maxX) maxX = x + 1;
                }
            }
        }

        if (!any) menuFont.glyphs[i].advanceX = 4;
        else menuFont.glyphs[i].advanceX = maxX + 1;
    }

    menuFont.texture = LoadTextureFromImage(image);
    SetTextureFilter(menuFont.texture, TEXTURE_FILTER_POINT);
    UnloadImage(image);
    fontOwned = true;
}

static const char *GlyphRows(char letter)
{
    switch (letter)
    {
        case 'A': return
            ".###."
            "#...#"
            "#...#"
            "#####"
            "#...#"
            "#...#"
            "#...#";
        case 'C': return
            ".###."
            "#...#"
            "#...."
            "#...."
            "#...."
            "#...#"
            ".###.";
        case 'D': return
            "####."
            "#...#"
            "#...#"
            "#...#"
            "#...#"
            "#...#"
            "####.";
        case 'E': return
            "#####"
            "#...."
            "#...."
            "####."
            "#...."
            "#...."
            "#####";
        case 'F': return
            "#####"
            "#...."
            "#...."
            "####."
            "#...."
            "#...."
            "#....";
        case 'I': return
            "#####"
            "..#.."
            "..#.."
            "..#.."
            "..#.."
            "..#.."
            "#####";
        case 'N': return
            "#...#"
            "##..#"
            "#.#.#"
            "#..##"
            "#...#"
            "#...#"
            "#...#";
        case 'O': return
            ".###."
            "#...#"
            "#...#"
            "#...#"
            "#...#"
            "#...#"
            ".###.";
        case 'P': return
            "####."
            "#...#"
            "#...#"
            "####."
            "#...."
            "#...."
            "#....";
        case 'R': return
            "####."
            "#...#"
            "#...#"
            "####."
            "#.#.."
            "#..#."
            "#...#";
        case 'T': return
            "#####"
            "..#.."
            "..#.."
            "..#.."
            "..#.."
            "..#.."
            "..#..";
        default: return NULL;
    }
}

static bool GlyphCell(const char *rows, int x, int y)
{
    if (rows == NULL) return false;
    if ((x < 0) || (y < 0) || (x >= 5) || (y >= 7)) return false;
    return rows[y*5 + x] == '#';
}

static int LogoAdvance(char letter, int cell, int gap)
{
    if (letter == ' ') return cell*3;
    if (GlyphRows(letter) == NULL) return cell*3;
    return cell*5 + gap;
}

static int LogoWidth(const char *text, int cell, int gap)
{
    int width = 0;
    int length = (int)strlen(text);

    for (int i = 0; i < length; i++)
    {
        width += LogoAdvance(text[i], cell, gap);
        if ((text[i] != ' ') && (i + 1 < length) && (text[i + 1] == ' ')) width -= gap;
    }

    if ((length > 0) && (text[length - 1] != ' ')) width -= gap;
    if (width < 0) width = 0;
    return width;
}

static void DrawStoneRect(int x, int y, int w, int h, Color tint)
{
    Rectangle dest = { (float)x, (float)y, (float)w, (float)h };

    if ((w <= 0) || (h <= 0)) return;

    if (stoneTexture.id == 0)
    {
        DrawRectangleRec(dest, tint);
        return;
    }

    DrawTexturePro(stoneTexture, (Rectangle){ 0, 0, 16, 16 }, dest, (Vector2){ 0, 0 }, 0.0f, tint);
}

static void DrawLogoWord(const char *text, int originX, int originY, int cell, int depth, int outline)
{
    int length = (int)strlen(text);
    int cursor = 0;
    Color shadow = { 0, 0, 0, 90 };
    Color frontTint = { 210, 210, 210, 255 };
    Color topTint = { 255, 255, 255, 255 };
    Color sideTint = { 70, 70, 70, 255 };

    for (int pass = 0; pass < 5; pass++)
    {
        cursor = 0;

        for (int i = 0; i < length; i++)
        {
            const char *rows = GlyphRows(text[i]);

            if (rows != NULL)
            {
                for (int gy = 0; gy < 7; gy++)
                {
                    for (int gx = 0; gx < 5; gx++)
                    {
                        if (!GlyphCell(rows, gx, gy)) continue;

                        int px = originX + cursor + gx*cell;
                        int py = originY + gy*cell;
                        bool left = !GlyphCell(rows, gx - 1, gy);
                        bool right = !GlyphCell(rows, gx + 1, gy);
                        bool above = !GlyphCell(rows, gx, gy - 1);
                        bool below = !GlyphCell(rows, gx, gy + 1);

                        if (pass == 0)
                        {
                            DrawRectangle(px + depth, py + depth, cell, cell, shadow);
                        }
                        else if ((pass == 1) && right)
                        {
                            int sideY = above ? (py - depth) : py;
                            int sideH = above ? (cell + depth) : cell;
                            DrawStoneRect(px + cell, sideY, depth, sideH, sideTint);
                        }
                        else if ((pass == 2) && above)
                        {
                            DrawStoneRect(px, py - depth, cell, depth, topTint);
                        }
                        else if (pass == 3)
                        {
                            int nudge = ((gx*3 + gy*5) & 7) - 3;
                            Color tint = frontTint;
                            tint.r = (unsigned char)(tint.r + nudge);
                            tint.g = (unsigned char)(tint.g + nudge);
                            tint.b = (unsigned char)(tint.b + nudge);
                            DrawStoneRect(px, py, cell, cell, tint);
                        }
                        else if (pass == 4)
                        {
                            if (above)
                            {
                                int topW = cell + (left ? outline : 0) + (right ? depth + outline : 0);
                                DrawRectangle(px - (left ? outline : 0), py - depth - outline, topW, outline, BLACK);
                            }
                            if (below)
                            {
                                int botW = cell + (left ? outline : 0) + (right ? depth + outline : 0);
                                DrawRectangle(px - (left ? outline : 0), py + cell, botW, outline, BLACK);
                            }
                            if (left)
                            {
                                int leftY = above ? (py - depth) : py;
                                int leftH = cell + (above ? depth : 0);
                                DrawRectangle(px - outline, leftY, outline, leftH, BLACK);
                            }
                            if (right)
                            {
                                int sideY = above ? (py - depth) : py;
                                int sideH = above ? (cell + depth) : cell;
                                DrawRectangle(px + cell + depth, sideY, outline, sideH, BLACK);
                            }
                        }
                    }
                }
            }

            cursor += LogoAdvance(text[i], cell, depth + outline);
            if ((text[i] != ' ') && (i + 1 < length) && (text[i + 1] == ' ')) cursor -= depth + outline;
        }
    }
}

static void DrawPanoramaFace(Texture2D texture, Vector3 bl, Vector3 br, Vector3 tr, Vector3 tl)
{
    if (texture.id == 0) return;

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(bl.x, bl.y, bl.z);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(br.x, br.y, br.z);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(tr.x, tr.y, tr.z);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(tl.x, tl.y, tl.z);
    rlEnd();
    rlSetTexture(0);
}

static void DrawPersonIcon(Rectangle bounds)
{
    float s = bounds.width/20.0f;
    float x = bounds.x;
    float y = bounds.y;
    Color ink = { 240, 240, 240, 255 };

    DrawRectangle((int)(x + 7*s), (int)(y + 3*s), (int)(6*s), (int)(6*s), ink);
    DrawRectangle((int)(x + 4*s), (int)(y + 10*s), (int)(12*s), (int)(3*s), ink);
    DrawRectangle((int)(x + 8*s), (int)(y + 10*s), (int)(4*s), (int)(5*s), ink);
    DrawRectangle((int)(x + 7*s), (int)(y + 15*s), (int)(2*s), (int)(3*s), ink);
    DrawRectangle((int)(x + 11*s), (int)(y + 15*s), (int)(2*s), (int)(3*s), ink);
}

//----------------------------------------------------------------------------------
// Menu UI
//----------------------------------------------------------------------------------
void InitMenuUi(void)
{
    if (menuReady) return;

    LoadMenuFont();
    for (int i = 0; i < 6; i++)
    {
        char path[96] = { 0 };

        snprintf(path, sizeof(path), "resources/textures/gui/title/background/panorama_%d.png", i);
        panorama[i] = LoadPanorama(path);
    }

    panoramaOverlay = LoadUiTexture("resources/textures/gui/title/background/panorama_overlay.png", TEXTURE_FILTER_BILINEAR);
    dirtTexture = LoadUiTexture("resources/textures/gui/options_background.png", TEXTURE_FILTER_POINT);
    widgetsTexture = LoadUiTexture("resources/textures/gui/widgets.png", TEXTURE_FILTER_POINT);
    stoneTexture = LoadNeutralStone();
    panoramaTimer = 0.0f;
    menuReady = true;
}

void UnloadMenuUi(void)
{
    if (!menuReady) return;

    if (fontOwned) UnloadFont(menuFont);
    for (int i = 0; i < 6; i++)
    {
        if (panorama[i].id != 0) UnloadTexture(panorama[i]);
        panorama[i] = (Texture2D){ 0 };
    }
    if (panoramaOverlay.id != 0) UnloadTexture(panoramaOverlay);
    if (dirtTexture.id != 0) UnloadTexture(dirtTexture);
    if (widgetsTexture.id != 0) UnloadTexture(widgetsTexture);
    if (stoneTexture.id != 0) UnloadTexture(stoneTexture);
    panoramaOverlay = (Texture2D){ 0 };
    dirtTexture = (Texture2D){ 0 };
    widgetsTexture = (Texture2D){ 0 };
    stoneTexture = (Texture2D){ 0 };
    fontOwned = false;
    menuReady = false;
}

void UpdateMenuUi(void)
{
    float dt = GetFrameTime();

    if (dt > 0.1f) dt = 0.1f;
    panoramaTimer += dt*60.0f;
}

int MenuScale(void)
{
    int sx = GetScreenWidth()/320;
    int sy = GetScreenHeight()/240;
    int scale = (sx < sy) ? sx : sy;

    if (scale < 1) scale = 1;
    if (scale > 4) scale = 4;
    return scale;
}

Font GetMenuFont(void)
{
    return menuFont;
}

void DrawPanoramaBackground(void)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    if (panorama[0].id == 0)
    {
        DrawRectangleGradientV(0, 0, screenW, screenH, (Color){ 86, 150, 196, 255 }, (Color){ 196, 112, 48, 255 });
        return;
    }

    float pitch = (sinf(panoramaTimer/400.0f)*25.0f + 20.0f)*DEG2RAD;
    float yaw = (-panoramaTimer*0.1f)*DEG2RAD;
    Camera3D camera = { 0 };
    float half = 8.0f;

    camera.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.target = (Vector3){ cosf(pitch)*sinf(yaw), sinf(pitch), cosf(pitch)*cosf(yaw) };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 100.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(camera);
        rlDisableBackfaceCulling();
        DrawPanoramaFace(panorama[0],
            (Vector3){ -half, -half, half }, (Vector3){ half, -half, half },
            (Vector3){ half, half, half }, (Vector3){ -half, half, half });
        DrawPanoramaFace(panorama[2],
            (Vector3){ half, -half, -half }, (Vector3){ -half, -half, -half },
            (Vector3){ -half, half, -half }, (Vector3){ half, half, -half });
        DrawPanoramaFace(panorama[1],
            (Vector3){ half, -half, half }, (Vector3){ half, -half, -half },
            (Vector3){ half, half, -half }, (Vector3){ half, half, half });
        DrawPanoramaFace(panorama[3],
            (Vector3){ -half, -half, -half }, (Vector3){ -half, -half, half },
            (Vector3){ -half, half, half }, (Vector3){ -half, half, -half });
        DrawPanoramaFace(panorama[4],
            (Vector3){ -half, half, half }, (Vector3){ half, half, half },
            (Vector3){ half, half, -half }, (Vector3){ -half, half, -half });
        DrawPanoramaFace(panorama[5],
            (Vector3){ -half, -half, -half }, (Vector3){ half, -half, -half },
            (Vector3){ half, -half, half }, (Vector3){ -half, -half, half });
    EndMode3D();
    rlEnableBackfaceCulling();

    if (panoramaOverlay.id != 0)
    {
        DrawTexturePro(panoramaOverlay, (Rectangle){ 0, 0, (float)panoramaOverlay.width, (float)panoramaOverlay.height },
            (Rectangle){ 0, 0, (float)screenW, (float)screenH }, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }
}

void DrawDirtBackground(void)
{
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int tile = 32*MenuScale();
    Color tint = { 64, 64, 64, 255 };

    if (tile < 16) tile = 16;

    if (dirtTexture.id == 0)
    {
        ClearBackground((Color){ 42, 28, 16, 255 });
        return;
    }

    for (int y = 0; y < screenH; y += tile)
    {
        for (int x = 0; x < screenW; x += tile)
        {
            DrawTexturePro(dirtTexture, (Rectangle){ 0, 0, 16, 16 },
                (Rectangle){ (float)x, (float)y, (float)tile, (float)tile },
                (Vector2){ 0, 0 }, 0.0f, tint);
        }
    }
}

void DrawMenuText(int x, int y, int fontSize, const char *text, Color color)
{
    int shadow = MenuScale();
    Font font = GetMenuFont();

    if (text == NULL) return;
    DrawTextEx(font, text, (Vector2){ (float)(x + shadow), (float)(y + shadow) }, (float)fontSize, 0.0f, (Color){ 0, 0, 0, 180 });
    DrawTextEx(font, text, (Vector2){ (float)x, (float)y }, (float)fontSize, 0.0f, color);
}

void DrawMenuTextCentered(int centerX, int y, int fontSize, const char *text, Color color)
{
    int width = MenuTextWidth(text, fontSize);
    DrawMenuText(centerX - width/2, y, fontSize, text, color);
}

int MenuTextWidth(const char *text, int fontSize)
{
    Vector2 size = MeasureTextEx(GetMenuFont(), text, (float)fontSize, 0.0f);
    return (int)size.x;
}

void DrawMenuTextField(Rectangle bounds, const char *text, bool focused, int frames)
{
    int scale = MenuScale();
    int fontSize = 8*scale;
    int pad = 4*scale;
    int textWidth = MenuTextWidth(text, fontSize);
    int maxWidth = (int)bounds.width - pad*2;
    int offset = 0;

    if (text == NULL) text = "";
    if (textWidth > maxWidth) offset = textWidth - maxWidth;

    DrawRectangleRec(bounds, BLACK);
    DrawRectangleLinesEx(bounds, (float)scale, WHITE);
    BeginScissorMode((int)bounds.x + pad, (int)bounds.y, (int)bounds.width - pad*2, (int)bounds.height);
        DrawTextEx(GetMenuFont(), text,
            (Vector2){ bounds.x + pad - offset, bounds.y + (bounds.height - fontSize)/2.0f },
            (float)fontSize, 0.0f, WHITE);
        if (focused && ((frames/20)%2 == 0))
        {
            DrawRectangle((int)(bounds.x + pad - offset + textWidth), (int)(bounds.y + scale*2), scale, (int)bounds.height - scale*4, WHITE);
        }
    EndScissorMode();
}

void UpdateMenuButton(MenuButton *button)
{
    Vector2 mouse = GetMousePosition();

    button->hovered = CheckCollisionPointRec(mouse, button->bounds);
    button->clicked = button->enabled && button->hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawStoneButton(const MenuButton *button, bool emphasized)
{
    int scale = MenuScale();
    bool hot = button->enabled && (button->hovered || emphasized);
    Color tint = { 176, 176, 176, 255 };
    int fontSize = 8*scale;
    Color textColor = { 48, 48, 48, 255 };

    if (!button->enabled) tint = (Color){ 112, 112, 112, 255 };
    else if (hot) tint = (Color){ 230, 230, 230, 255 };

    BeginScissorMode((int)button->bounds.x, (int)button->bounds.y, (int)button->bounds.width, (int)button->bounds.height);
        if (stoneTexture.id == 0)
        {
            DrawRectangleRec(button->bounds, tint);
        }
        else
        {
            int tile = 16*scale;
            if (tile < 8) tile = 8;
            for (int y = (int)button->bounds.y; y < (int)(button->bounds.y + button->bounds.height); y += tile)
            {
                for (int x = (int)button->bounds.x; x < (int)(button->bounds.x + button->bounds.width); x += tile)
                {
                    DrawTexturePro(stoneTexture, (Rectangle){ 0, 0, 16, 16 },
                        (Rectangle){ (float)x, (float)y, (float)tile, (float)tile },
                        (Vector2){ 0, 0 }, 0.0f, tint);
                }
            }
        }
    EndScissorMode();

    DrawRectangle((int)button->bounds.x, (int)button->bounds.y, (int)button->bounds.width, scale, (Color){ 255, 255, 255, hot ? 210 : 110 });
    DrawRectangle((int)button->bounds.x, (int)(button->bounds.y + button->bounds.height) - scale, (int)button->bounds.width, scale, (Color){ 0, 0, 0, 120 });
    DrawRectangleLinesEx(button->bounds, (float)((scale > 2) ? 2 : 1), BLACK);
    if (hot) DrawRectangleLinesEx(button->bounds, (float)((scale > 2) ? 2 : 1), WHITE);

    if (!button->enabled) textColor = (Color){ 160, 160, 160, 255 };
    if ((button->label != NULL) && (button->label[0] != '\0'))
    {
        int textWidth = MenuTextWidth(button->label, fontSize);
        int tx = (int)(button->bounds.x + button->bounds.width/2.0f - textWidth/2.0f);
        int ty = (int)(button->bounds.y + button->bounds.height/2.0f - fontSize/2.0f);
        DrawTextEx(GetMenuFont(), button->label, (Vector2){ (float)tx, (float)ty }, (float)fontSize, 0.0f, textColor);
    }
}

void DrawLanguageButton(const MenuButton *button, bool emphasized)
{
    bool hot = button->enabled && (button->hovered || emphasized);
    Rectangle source = hot ? (Rectangle){ 0, 128, 20, 20 } : (Rectangle){ 0, 106, 20, 20 };

    if (widgetsTexture.id == 0)
    {
        DrawStoneButton(button, emphasized);
        return;
    }

    DrawTexturePro(widgetsTexture, source, button->bounds, (Vector2){ 0, 0 }, 0.0f, WHITE);
}

void DrawAccessibilityButton(const MenuButton *button, bool emphasized)
{
    DrawStoneButton(button, emphasized);
    DrawPersonIcon(button->bounds);
}

Rectangle DrawOpenCraftLogo(int centerX, int topY, int scale)
{
    int cell = 6*scale;
    int depth = cell/2;
    int outline = (cell >= 12) ? cell/6 : 2;
    int editionCell = 3*scale;
    int editionDepth = 0;
    int editionOutline = 1;
    int width = 0;
    int editionWidth = 0;
    Rectangle bounds = { 0 };

    if (outline < 2) outline = 2;
    if (editionCell < 4) editionCell = 4;
    editionDepth = editionCell/2;
    editionOutline = (editionCell >= 8) ? 2 : 1;

    width = LogoWidth("OPENCRAFT", cell, depth + outline);
    editionWidth = LogoWidth("OPEN EDITION", editionCell, editionDepth + editionOutline);
    bounds = (Rectangle){ (float)(centerX - width/2), (float)topY, (float)width, (float)(cell*7) };

    DrawLogoWord("OPENCRAFT", (int)bounds.x, topY, cell, depth, outline);
    DrawLogoWord("OPEN EDITION", centerX - editionWidth/2, topY + cell*7 + 4*scale, editionCell, editionDepth, editionOutline);
    return bounds;
}

void SetUnavailableMenu(UnavailableMenu menu)
{
    unavailableMenu = menu;
}

UnavailableMenu GetUnavailableMenu(void)
{
    return unavailableMenu;
}
