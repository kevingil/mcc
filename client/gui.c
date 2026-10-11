#include "gui.h"
#include "menu_ui.h"
#include "voxel_renderer.h"

#include "rlgl.h"

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    #include <GLES2/gl2.h>
#else
    #include <GL/gl.h>
#endif

#include <stddef.h>

//----------------------------------------------------------------------------------
// Sprite atlas
//----------------------------------------------------------------------------------
#define ATLAS_WIDTH 512
#define ATLAS_HEIGHT 256

typedef struct {
    int x;
    int y;
    int w;
    int h;
} SpriteRect;

typedef struct {
    unsigned int base;
    const unsigned int *variants;
    int count;
    int percent;
} Speckle;

typedef struct {
    char key;
    unsigned int rgb;
    unsigned char alpha;
} PaletteEntry;

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
static const char *maskCode =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "void main()\n"
    "{\n"
    "    if (texture2D(texture0, fragTexCoord).a < 0.1) discard;\n"
    "    gl_FragColor = vec4(1.0);\n"
    "}\n";
#else
static const char *maskCode =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    if (texture(texture0, fragTexCoord).a < 0.1) discard;\n"
    "    finalColor = vec4(1.0);\n"
    "}\n";
#endif

static bool guiReady = false;
static Texture2D atlas = { 0 };
static Texture2D chestTexture = { 0 };
static Shader maskShader = { 0 };
static SpriteRect sprites[GUI_SPRITE_COUNT] = { 0 };

static Color *canvas = NULL;
static int canvasX = 0;
static int canvasY = 0;
static int packX = 0;
static int packY = 0;
static int packRow = 0;

static const unsigned int darkVariants[] = { 0x5A5A5A, 0x606060, 0x626262, 0x666666, 0x676767, 0x565656 };
static const unsigned int lightVariants[] = { 0x7A7A7A, 0x808080, 0x737373, 0x777777, 0x858585, 0x898989, 0x848484, 0x7C7C7C };
static const unsigned int oliveVariants[] = { 0x252404, 0x2A2607, 0x2C2808, 0x2E2B0A, 0x1B1D04, 0x191A04, 0x1D2004, 0x202205, 0x302C0E };
static const unsigned int paleVariants[] = { 0xCDE0C8, 0xDCEED7, 0xC6D9C1, 0xE2F3DE };

static const Speckle darkSpeckle = { 0x5D5D5D, darkVariants, 6, 35 };
static const Speckle lightSpeckle = { 0x7E7E7E, lightVariants, 8, 15 };
static const Speckle oliveSpeckle = { 0x272505, oliveVariants, 9, 35 };
static const Speckle paleSpeckle = { 0xD5E8D0, paleVariants, 4, 25 };

static Color Rgb(unsigned int rgb)
{
    return (Color){ (unsigned char)((rgb >> 16) & 0xFF), (unsigned char)((rgb >> 8) & 0xFF), (unsigned char)(rgb & 0xFF), 255 };
}

static Color Rgba(unsigned int rgb, unsigned char alpha)
{
    Color color = Rgb(rgb);

    color.a = alpha;
    return color;
}

static unsigned int Hash(int x, int y, int salt)
{
    unsigned int h = (unsigned int)x*374761393u + (unsigned int)y*668265263u + (unsigned int)salt*2246822519u;

    h = (h ^ (h >> 13))*1274126177u;
    return h ^ (h >> 16);
}

static Color Speck(const Speckle *speckle, int x, int y, int salt)
{
    unsigned int h = Hash(x, y, salt);

    if ((int)(h%100u) < speckle->percent) return Rgb(speckle->variants[(h/100u)%(unsigned int)speckle->count]);
    return Rgb(speckle->base);
}

static void Reserve(GuiSprite sprite, int w, int h)
{
    if (packX + w > ATLAS_WIDTH)
    {
        packX = 0;
        packY += packRow + 1;
        packRow = 0;
    }
    sprites[sprite] = (SpriteRect){ packX, packY, w, h };
    canvasX = packX;
    canvasY = packY;
    packX += w + 1;
    if (h > packRow) packRow = h;
}

static void Put(int x, int y, Color color)
{
    int px = canvasX + x;
    int py = canvasY + y;

    if ((px < 0) || (py < 0) || (px >= ATLAS_WIDTH) || (py >= ATLAS_HEIGHT)) return;
    canvas[py*ATLAS_WIDTH + px] = color;
}

// Inclusive on both corners.
static void Box(int x0, int y0, int x1, int y1, Color color)
{
    for (int y = y0; y <= y1; y++)
    {
        for (int x = x0; x <= x1; x++) Put(x, y, color);
    }
}

static void PaintRows(const char *const *rows, int rowCount, int ox, int oy, const PaletteEntry *palette, int paletteCount)
{
    for (int y = 0; y < rowCount; y++)
    {
        for (int x = 0; rows[y][x] != '\0'; x++)
        {
            for (int i = 0; i < paletteCount; i++)
            {
                if (palette[i].key == rows[y][x]) Put(ox + x, oy + y, Rgba(palette[i].rgb, palette[i].alpha));
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Sprite painters
//----------------------------------------------------------------------------------
static void PaintSlotStrip(int ox, int oy, int slots)
{
    int w = 20*slots + 2;
    Color black = Rgb(0x000000);

    Box(ox, oy, ox + w - 1, oy, black);
    Box(ox, oy + 21, ox + w - 1, oy + 21, black);
    Box(ox, oy, ox, oy + 21, black);
    Box(ox + w - 1, oy, ox + w - 1, oy + 21, black);
    Box(ox + 1, oy + 1, ox + w - 3, oy + 1, Rgb(0x939393));
    Put(ox + w - 2, oy + 1, Rgb(0x6D6D6D));
    Box(ox + 1, oy + 2, ox + 1, oy + 20, Rgb(0x939393));
    Box(ox + w - 2, oy + 2, ox + w - 2, oy + 20, Rgb(0x474747));
    Put(ox + 1, oy + 1, Rgb(0xC8C8C8));
    Put(ox + 2, oy + 1, Rgb(0xA8A8A8));
    Put(ox + 1, oy + 2, Rgb(0xA8A8A8));

    for (int k = 0; k < slots; k++)
    {
        int sx = 20*k + 1;

        if (k > 0)
        {
            for (int y = 2; y <= 19; y++) Put(ox + sx, oy + y, Speck(&lightSpeckle, sx, y, 1));
            Put(ox + sx, oy + 20, Speck(&darkSpeckle, sx, 20, 2));
        }
        for (int y = 2; y <= 18; y++) Put(ox + sx + 1, oy + y, Speck(&darkSpeckle, sx + 1, y, 2));
        Put(ox + sx + 1, oy + 19, Rgb(0x707070));
        for (int x = sx + 2; x <= sx + 17; x++) Put(ox + x, oy + 2, Speck(&darkSpeckle, x, 2, 2));
        Put(ox + sx + 18, oy + 2, Rgb(0x737373));
        for (int y = 3; y <= 18; y++)
        {
            for (int x = sx + 2; x <= sx + 17; x++)
            {
                Color color = ((y == 3) || (x == sx + 2))? Rgb(0x040404) : Speck(&oliveSpeckle, x, y, 3);

                color.a = 186;
                Put(ox + x, oy + y, color);
            }
        }
        for (int y = 3; y <= 19; y++) Put(ox + sx + 18, oy + y, Speck(&lightSpeckle, sx + 18, y, 1));
        if (k < slots - 1)
        {
            for (int y = 2; y <= 20; y++) Put(ox + sx + 19, oy + y, Speck(&darkSpeckle, sx + 19, y, 2));
        }
        for (int x = sx + 2; x <= sx + 17; x++) Put(ox + x, oy + 19, Speck(&lightSpeckle, x, 19, 1));
        for (int x = sx + 1; x <= sx + 18; x++) Put(ox + x, oy + 20, Speck(&darkSpeckle, x, 20, 2));
    }
}

static void PaintSelection(void)
{
    Color edge = Rgba(0x000000, 168);
    Color mid = Rgb(0xA1B29D);
    Color dark = Rgb(0x5F6D5C);
    Color glint = Rgb(0xF4FFF1);

    Box(0, 0, 23, 0, Rgb(0x000000));
    Box(0, 1, 0, 22, edge);
    Box(23, 1, 23, 22, edge);
    for (int x = 1; x <= 21; x++) Put(x, 1, Speck(&paleSpeckle, x, 1, 4));
    for (int y = 2; y <= 21; y++) Put(1, y, Speck(&paleSpeckle, 1, y, 4));
    Put(22, 1, mid);
    Put(1, 22, mid);
    Box(22, 2, 22, 22, dark);
    Box(2, 22, 21, 22, dark);
    Box(2, 2, 2, 21, mid);
    Box(2, 2, 21, 2, mid);
    Box(21, 3, 21, 21, mid);
    Box(2, 21, 21, 21, mid);
    Box(3, 3, 19, 3, dark);
    Box(3, 4, 3, 19, dark);
    Put(20, 3, Rgb(0xA8B9A4));
    for (int y = 4; y <= 19; y++) Put(20, y, Speck(&paleSpeckle, 20, y, 5));
    for (int x = 3; x <= 18; x++) Put(x, 20, Speck(&paleSpeckle, x, 20, 5));
    Put(19, 20, glint);
    Put(20, 20, glint);
    Box(1, 1, 4, 1, glint);
    Box(1, 2, 1, 4, glint);
}

static void PaintAttack(Color handle, Color blade)
{
    Box(0, 1, 2, 2, handle);
    Box(3, 0, 3, 3, handle);
    Box(4, 1, 15, 1, blade);
    Box(4, 2, 13, 2, blade);
}

static const char *const heartRows[] = {
    "..##.##..",
    ".#st#ss#.",
    "#sthssss#",
    "#shsssss#",
    "#ussssku#",
    ".#ussku#.",
    "..#uku#..",
    "...#u#...",
    "....#...."
};

static const char *const foodRows[] = {
    "..##.....",
    ".#ab#....",
    "#ahac#...",
    "#racck#..",
    ".#dckk#..",
    "..#ddk#..",
    "...###e#.",
    "......#f#",
    ".......#."
};

static const char *const airRows[] = {
    ".........",
    "...aab...",
    "..a...b..",
    ".a.hh..c.",
    ".a.h...c.",
    ".b.....c.",
    "..b...c..",
    "...ccc...",
    "........."
};

static const char *const airBurstRows[] = {
    ".........",
    ".........",
    "...a.b...",
    "..a...b..",
    "....h....",
    "..b...c..",
    "...b.c...",
    ".........",
    "........."
};

static void PaintIcon(const char *const *rows, const PaletteEntry *palette, int paletteCount, bool outlineOnly, Color fill)
{
    for (int y = 0; y < 9; y++)
    {
        for (int x = 0; x < 9; x++)
        {
            char key = rows[y][x];

            if (key == '.') continue;
            if (outlineOnly)
            {
                Put(x, y, (key == '#')? Rgb(0x000000) : fill);
                continue;
            }
            for (int i = 0; i < paletteCount; i++)
            {
                if ((palette[i].key == key) && (key != '#')) Put(x, y, Rgba(palette[i].rgb, palette[i].alpha));
            }
        }
    }
}

static void PaintExperienceBar(void)
{
    Color outline = Rgb(0x0A130E);

    Box(1, 0, 180, 0, outline);
    Box(1, 4, 180, 4, outline);
    Box(0, 1, 0, 3, outline);
    Box(181, 1, 181, 3, outline);
    for (int x = 1; x <= 180; x++)
    {
        int segment = (x <= 89)? (x - 1)%10 : (x - 92)%10;
        bool divider = (segment == 9) || (x == 90) || (x == 91);

        if (divider)
        {
            Color line = ((x == 90) || (x == 91))? Rgb(0x1E2A24) : Rgb(0x131E18);

            Box(x, 1, x, 3, line);
            continue;
        }
        {
            int edge = (segment < 4)? segment : 8 - segment;
            unsigned int outer = (edge == 0)? 0x35423B : ((edge == 1)? 0x2D3933 : 0x28342E);
            unsigned int inner = (edge == 0)? 0x26322B : ((edge == 1)? 0x1F2B24 : 0x19241E);

            Put(x, 1, Rgb(outer));
            Put(x, 2, Rgb(inner));
            Put(x, 3, Rgb(outer));
        }
    }
}

// Same bevel as GuiDrawInset(): dark top-left, white bottom-right.
static void PaintInset(int w, int h, Color fill)
{
    Box(0, 0, w - 2, 0, Rgb(0x373737));
    Put(w - 1, 0, Rgb(0x8B8B8B));
    Box(0, 1, 0, h - 2, Rgb(0x373737));
    Box(1, 1, w - 2, h - 2, fill);
    Box(w - 1, 1, w - 1, h - 2, Rgb(0xFFFFFF));
    Put(0, h - 1, Rgb(0x8B8B8B));
    Box(1, h - 1, w - 1, h - 1, Rgb(0xFFFFFF));
}

static void PaintScroller(unsigned int light, unsigned int fill, unsigned int shadow, unsigned int grip)
{
    Box(0, 0, 10, 0, Rgb(light));
    Put(11, 0, Rgb(grip));
    Box(0, 1, 0, 13, Rgb(light));
    Box(1, 1, 10, 13, Rgb(fill));
    Box(11, 1, 11, 13, Rgb(shadow));
    for (int y = 2; y <= 12; y += 2) Box(2, y, 9, y, Rgb(grip));
    Put(0, 14, Rgb(grip));
    Box(1, 14, 11, 14, Rgb(shadow));
}

// One row of a tab's side walls: black edge, white highlight, fill, shadow, black edge.
static void TabBody(int y, Color fill)
{
    Put(0, y, Rgb(0x000000));
    Box(1, y, 2, y, Rgb(0xFFFFFF));
    Box(3, y, 22, y, fill);
    Box(23, y, 24, y, Rgb(0x555555));
    Put(25, y, Rgb(0x000000));
}

static void PaintTopTab(bool selected, int variant)
{
    Color black = Rgb(0x000000);
    Color white = Rgb(0xFFFFFF);
    Color light = Rgb(0xC6C6C6);
    Color shadow = Rgb(0x555555);
    Color fill = selected? light : Rgb(0x8B8B8B);
    int top = selected? 0 : 2;

    Box(2, top, 22, top, black);
    Put(1, top + 1, black);
    Box(2, top + 1, 22, top + 1, white);
    Put(23, top + 1, black);
    Put(0, top + 2, black);
    Box(1, top + 2, 22, top + 2, white);
    Put(23, top + 2, fill);
    Put(24, top + 2, black);
    Put(0, top + 3, black);
    Box(1, top + 3, 3, top + 3, white);
    Box(4, top + 3, 22, top + 3, fill);
    Box(23, top + 3, 24, top + 3, shadow);
    Put(25, top + 3, black);
    if (!selected)
    {
        for (int y = 6; y <= 31; y++) TabBody(y, fill);
        return;
    }
    for (int y = 4; y <= 28; y++) TabBody(y, fill);
    for (int y = 29; y <= 31; y++)
    {
        if (variant == 0)
        {
            Put(0, y, black);
            Box(1, y, 2, y, white);
        }
        else if (y < 31) Box(0, y, 2, y, white);
        else Box(0, y, 2, y, light);
        Box(3, y, 22, y, light);
        if (variant == 2)
        {
            Box(23, y, 24, y, shadow);
            Put(25, y, black);
        }
        else if (y < 31)
        {
            Box(23, y, 24, y, shadow);
            Put(25, y, white);
        }
        else
        {
            Put(23, y, shadow);
            Box(24, y, 25, y, light);
        }
    }
}

static void PaintBottomTab(bool selected, int variant)
{
    Color black = Rgb(0x000000);
    Color white = Rgb(0xFFFFFF);
    Color light = Rgb(0xC6C6C6);
    Color shadow = Rgb(0x555555);
    Color fill = selected? light : Rgb(0x8B8B8B);
    int bottom = selected? 31 : 27;

    if (!selected)
    {
        for (int y = 0; y <= 24; y++) TabBody(y, fill);
    }
    else
    {
        for (int y = 3; y <= 28; y++) TabBody(y, fill);
        if (variant == 0)
        {
            for (int y = 0; y <= 2; y++)
            {
                Put(0, y, black);
                Box(1, y, 2, y, white);
            }
            Box(3, 0, 25, 0, light);
        }
        else
        {
            Box(0, 0, 22, 0, light);
            Box(0, 1, 1, 1, shadow);
            Put(2, 1, white);
            Put(0, 2, shadow);
            Box(1, 2, 2, 2, white);
            if (variant == 1) Box(23, 0, 25, 0, light);
        }
        Box(3, 1, 22, 2, light);
        if (variant == 2)
        {
            Box(23, 0, 24, 2, shadow);
            Box(25, 0, 25, 2, black);
        }
        else Box(23, 1, 25, 2, shadow);
    }
    Put(1, bottom - 2, black);
    Put(2, bottom - 2, fill);
    Box(3, bottom - 2, 24, bottom - 2, shadow);
    Put(25, bottom - 2, black);
    Put(2, bottom - 1, black);
    Box(3, bottom - 1, 23, bottom - 1, shadow);
    Put(24, bottom - 1, black);
    Box(3, bottom, 23, bottom, black);
}

static const char *const bookRows[] = {
    "......oooo.....",
    "....ooghhgoo...",
    "..ooggghgggGoo.",
    "oogggggggggGGGo",
    "oGggggggggGGGdo",
    "oGGgggggggGGddo",
    "opGGGgggGGGddPo",
    "oppPGGGGGGddPPo",
    ".oopPPGGGddPPo.",
    "...oopPGdPPoo..",
    ".....ooGdoo....",
    ".......oo......"
};

static void PaintRecipeButton(bool highlighted)
{
    static const PaletteEntry bookPalette[] = {
        { 'o', 0x1E2B14, 255 }, { 'g', 0x58B154, 255 }, { 'G', 0x3E8A3B, 255 }, { 'h', 0x8ED68A, 255 },
        { 'd', 0x24532A, 255 }, { 'p', 0xE4E4E4, 255 }, { 'P', 0xA9A9A9, 255 }
    };
    Color edge = highlighted? Rgb(0xFFFFFF) : Rgb(0x000000);

    Box(2, 0, 17, 0, edge);
    Put(1, 1, edge);
    Box(2, 1, 17, 1, Rgb(0xFFFFFF));
    Put(18, 1, edge);
    for (int y = 2; y <= 15; y++)
    {
        Put(0, y, edge);
        Put(1, y, Rgb(0xFFFFFF));
        Box(2, y, 17, y, Rgb(0xC6C6C6));
        Put(18, y, Rgb(0x555555));
        Put(19, y, edge);
    }
    Put(1, 16, edge);
    Box(2, 16, 17, 16, Rgb(0x555555));
    Put(18, 16, edge);
    Box(2, 17, 17, 17, edge);
    PaintRows(bookRows, 12, 2, 3, bookPalette, (int)(sizeof(bookPalette)/sizeof(bookPalette[0])));
}

static const char *const helmetRows[] = {
    "................",
    "................",
    "................",
    "................",
    ".....oooooo.....",
    "....o......o....",
    "...o........o...",
    "...o........o...",
    "...o..oooo..o...",
    "...o.o....o.o...",
    "...o.o....o.o...",
    "...ooo....ooo..."
};

static const char *const chestplateRows[] = {
    "................",
    "................",
    "...oo......oo...",
    "..o..o....o..o..",
    ".o....oooo....o.",
    ".o............o.",
    ".oo..........oo.",
    "..oo........oo..",
    "...o........o...",
    "...o........o...",
    "...o........o...",
    "...o........o...",
    "...o........o...",
    "...oooooooooo..."
};

static const char *const leggingsRows[] = {
    "................",
    "................",
    "...oooooooooo...",
    "...o........o...",
    "...o........o...",
    "...o...oo...o...",
    "...o..o..o..o...",
    "...o..o..o..o...",
    "...o..o..o..o...",
    "...o..o..o..o...",
    "...o..o..o..o...",
    "...o..o..o..o...",
    "...o..o..o..o...",
    "...oooo..oooo..."
};

static const char *const bootsRows[] = {
    "................",
    "................",
    "................",
    "................",
    "................",
    "..oooo....oooo..",
    "..o..o....o..o..",
    "..o..o....o..o..",
    "..o..o....o..o..",
    "..o..oo...o..oo.",
    "..o...o...o...o.",
    "..o...o...o...o.",
    "..ooooo...ooooo."
};

static const char *const shieldRows[] = {
    "................",
    ".......oo.......",
    ".....oo..oo.....",
    "..ooo......ooo..",
    "..o..........o..",
    "..o..........o..",
    "...o........o...",
    "...o........o...",
    "...o........o...",
    "....o......o....",
    "....o......o....",
    ".....o....o.....",
    "......o..o......",
    ".......oo......."
};

static void PaintEmptySlotIcon(const char *const *rows, int rowCount)
{
    static const PaletteEntry outline[] = { { 'o', 0x555555, 255 } };

    PaintRows(rows, rowCount, 0, 0, outline, 1);
}

static void PaintDestroySlot(void)
{
    Color cross = Rgb(0x1F1F1F);

    PaintInset(18, 18, Rgb(0xAB7F7F));
    for (int y = 3; y <= 13; y++)
    {
        int d = (y > 8)? y - 8 : 8 - y;

        Put(8 - d, y, cross);
        Put(9 - d, y, cross);
        Put(8 + d, y, cross);
        Put(9 + d, y, cross);
    }
}

// Recipe book tabs stick out of the book's left edge. The selected one is
// drawn two pixels further left and its last three columns open the book's
// border so the tab and the page read as one piece.
static void PaintBookTab(bool selected)
{
    Color black = Rgb(0x000000);
    Color white = Rgb(0xFFFFFF);
    Color light = Rgb(0xC6C6C6);
    Color shadow = Rgb(0x555555);
    Color fill = selected? light : Rgb(0x8B8B8B);
    int right = selected? 31 : 29;

    Box(2, 0, right, 0, black);
    Put(1, 1, black);
    Box(2, 1, right, 1, white);
    Put(0, 2, black);
    Box(1, 2, right, 2, white);
    for (int y = 3; y <= 23; y++)
    {
        Put(0, y, black);
        Box(1, y, 2, y, white);
        Box(3, y, right, y, fill);
    }
    Put(0, 24, black);
    Put(1, 24, white);
    Put(2, 24, fill);
    Box(3, 24, right, 25, shadow);
    Put(1, 25, black);
    Put(2, 25, shadow);
    Box(2, 26, right, 26, black);
    if (!selected) return;
    Put(32, 0, black);
    Box(33, 0, 34, 0, white);
    Box(32, 1, 34, 2, white);
    Box(32, 3, 34, 25, light);
    Put(32, 26, black);
    Box(33, 26, 34, 26, white);
}

static void PaintRecipeSlot(unsigned int light, unsigned int fill, unsigned int shadow)
{
    Box(0, 0, 24, 24, Rgb(fill));
    Box(0, 0, 23, 0, Rgb(light));
    Box(0, 1, 0, 23, Rgb(light));
    Box(1, 24, 24, 24, Rgb(shadow));
    Box(24, 1, 24, 23, Rgb(shadow));
}

// A collection with alternatives shows a second card peeking out two pixels
// below and to the right of the front one.
static void PaintRecipeSlotStack(unsigned int light, unsigned int fill, unsigned int shadow)
{
    Box(2, 2, 24, 24, Rgb(fill));
    Box(2, 2, 23, 2, Rgb(light));
    Box(2, 3, 2, 23, Rgb(light));
    Box(3, 24, 24, 24, Rgb(shadow));
    Box(24, 3, 24, 23, Rgb(shadow));
    Box(0, 0, 22, 22, Rgb(fill));
    Box(0, 0, 21, 0, Rgb(light));
    Box(0, 1, 0, 21, Rgb(light));
    Box(1, 22, 22, 22, Rgb(shadow));
    Box(22, 1, 22, 21, Rgb(shadow));
}

static void PaintOverlayButton(unsigned int light, unsigned int fill, unsigned int shadow, bool highlighted)
{
    Box(0, 0, 23, 23, Rgb(fill));
    Box(0, 0, 22, 0, Rgb(light));
    Box(0, 1, 0, 22, Rgb(light));
    Box(1, 23, 23, 23, Rgb(shadow));
    Box(23, 1, 23, 22, Rgb(shadow));
    if (!highlighted) return;
    Box(0, 0, 23, 0, Rgb(0xFFFFFF));
    Box(0, 23, 23, 23, Rgb(0xFFFFFF));
    Box(0, 1, 0, 22, Rgb(0xFFFFFF));
    Box(23, 1, 23, 22, Rgb(0xFFFFFF));
}

// A small button with a 3x3 grid: gray cells show every recipe, green cells
// only the ones the inventory can craft.
static void PaintFilterButton(bool craftableOnly, bool highlighted)
{
    Color edge = highlighted? Rgb(0xFFFFFF) : Rgb(0x000000);
    Color cell = craftableOnly? Rgb(0x5DBA4F) : Rgb(0x8B8B8B);

    Box(1, 0, 24, 0, edge);
    Box(0, 1, 0, 14, edge);
    Box(25, 1, 25, 14, edge);
    Box(1, 15, 24, 15, edge);
    Box(1, 1, 23, 1, Rgb(0xFFFFFF));
    Box(1, 2, 1, 13, Rgb(0xFFFFFF));
    Box(2, 2, 23, 13, Rgb(0xC6C6C6));
    Box(24, 1, 24, 14, Rgb(0x555555));
    Box(1, 14, 23, 14, Rgb(0x555555));
    Box(8, 3, 17, 12, Rgb(0x373737));
    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++) Box(9 + 3*col, 4 + 3*row, 10 + 3*col, 5 + 3*row, cell);
    }
}

static void PaintPageArrow(bool forward, bool highlighted)
{
    Color outline = highlighted? Rgb(0x000000) : Rgb(0x373737);
    Color body = highlighted? Rgb(0xFFFFFF) : Rgb(0xB0B0B0);

    for (int y = 0; y < 17; y++)
    {
        int half = (y > 8)? 16 - y : y;

        for (int x = 0; x <= half; x++)
        {
            int px = forward? 2 + x : 9 - x;
            bool edge = (x == 0) || (x == half);

            Put(px, y, edge? outline : body);
        }
    }
}

static void PaintAtlas(void)
{
    static const PaletteEntry heartPalette[] = {
        { 's', 0xFF1313, 255 }, { 't', 0xFFC8C8, 255 }, { 'h', 0xFF7676, 255 }, { 'u', 0xBB1313, 255 }, { 'k', 0xD81414, 255 }
    };
    static const PaletteEntry foodPalette[] = {
        { 'a', 0xD8452C, 255 }, { 'b', 0xB8231A, 255 }, { 'h', 0xF0B898, 255 }, { 'r', 0xA82418, 255 },
        { 'c', 0xB57E4F, 255 }, { 'k', 0x8F5F36, 255 }, { 'd', 0x5E3A1A, 255 }, { 'e', 0xE6DAB2, 255 }, { 'f', 0xFFF9E6, 255 }
    };
    static const PaletteEntry airPalette[] = {
        { 'a', 0x1A8EF5, 255 }, { 'b', 0x3AA2FF, 255 }, { 'c', 0x63C0FF, 255 }, { 'h', 0xDAF0FF, 255 }
    };
    int heartCount = (int)(sizeof(heartPalette)/sizeof(heartPalette[0]));
    int foodCount = (int)(sizeof(foodPalette)/sizeof(foodPalette[0]));
    int airCount = (int)(sizeof(airPalette)/sizeof(airPalette[0]));

    Reserve(GUI_SPRITE_HOTBAR, 182, 22);
    PaintSlotStrip(0, 0, 9);
    Reserve(GUI_SPRITE_HOTBAR_SELECTION, 24, 23);
    PaintSelection();
    Reserve(GUI_SPRITE_HOTBAR_OFFHAND, 29, 24);
    PaintSlotStrip(0, 1, 1);
    Reserve(GUI_SPRITE_CROSSHAIR, 15, 15);
    Box(7, 3, 7, 11, Rgb(0xFFFFFF));
    Box(3, 7, 11, 7, Rgb(0xFFFFFF));
    Reserve(GUI_SPRITE_ATTACK_BACKGROUND, 16, 4);
    PaintAttack(Rgb(0x4A4A4A), Rgb(0x3C3C3C));
    Reserve(GUI_SPRITE_ATTACK_PROGRESS, 16, 4);
    PaintAttack(Rgb(0xE0E0E0), Rgb(0xFFFFFF));

    Reserve(GUI_SPRITE_HEART_CONTAINER, 9, 9);
    PaintIcon(heartRows, NULL, 0, true, Rgb(0x2A2A2A));
    Reserve(GUI_SPRITE_HEART_FULL, 9, 9);
    PaintIcon(heartRows, heartPalette, heartCount, false, BLANK);
    Reserve(GUI_SPRITE_FOOD_EMPTY, 9, 9);
    PaintIcon(foodRows, NULL, 0, true, Rgb(0x2E2A26));
    Reserve(GUI_SPRITE_FOOD_FULL, 9, 9);
    PaintIcon(foodRows, foodPalette, foodCount, false, BLANK);
    Reserve(GUI_SPRITE_AIR, 9, 9);
    PaintIcon(airRows, airPalette, airCount, false, BLANK);
    Reserve(GUI_SPRITE_AIR_BURSTING, 9, 9);
    PaintIcon(airBurstRows, airPalette, airCount, false, BLANK);
    Reserve(GUI_SPRITE_XP_BACKGROUND, 182, 5);
    PaintExperienceBar();

    Reserve(GUI_SPRITE_SLOT, 18, 18);
    PaintInset(18, 18, Rgb(0x8B8B8B));
    Reserve(GUI_SPRITE_SLOT_LARGE, 26, 26);
    PaintInset(26, 26, Rgb(0x8B8B8B));
    Reserve(GUI_SPRITE_SCROLLER, 12, 15);
    PaintScroller(0xFFFFFF, 0xC6C6C6, 0x555555, 0x8B8B8B);
    Reserve(GUI_SPRITE_SCROLLER_DISABLED, 12, 15);
    PaintScroller(0xC6C6C6, 0xA0A0A0, 0x555555, 0x7F7F7F);

    Reserve(GUI_SPRITE_TAB_TOP, 26, 32);
    PaintTopTab(false, 1);
    Reserve(GUI_SPRITE_TAB_TOP_SELECTED_FIRST, 26, 32);
    PaintTopTab(true, 0);
    Reserve(GUI_SPRITE_TAB_TOP_SELECTED, 26, 32);
    PaintTopTab(true, 1);
    Reserve(GUI_SPRITE_TAB_TOP_SELECTED_LAST, 26, 32);
    PaintTopTab(true, 2);
    Reserve(GUI_SPRITE_TAB_BOTTOM, 26, 32);
    PaintBottomTab(false, 1);
    Reserve(GUI_SPRITE_TAB_BOTTOM_SELECTED_FIRST, 26, 32);
    PaintBottomTab(true, 0);
    Reserve(GUI_SPRITE_TAB_BOTTOM_SELECTED, 26, 32);
    PaintBottomTab(true, 1);
    Reserve(GUI_SPRITE_TAB_BOTTOM_SELECTED_LAST, 26, 32);
    PaintBottomTab(true, 2);

    Reserve(GUI_SPRITE_RECIPE_BUTTON, 20, 18);
    PaintRecipeButton(false);
    Reserve(GUI_SPRITE_RECIPE_BUTTON_HIGHLIGHTED, 20, 18);
    PaintRecipeButton(true);

    Reserve(GUI_SPRITE_EMPTY_HELMET, 16, 16);
    PaintEmptySlotIcon(helmetRows, (int)(sizeof(helmetRows)/sizeof(helmetRows[0])));
    Reserve(GUI_SPRITE_EMPTY_CHESTPLATE, 16, 16);
    PaintEmptySlotIcon(chestplateRows, (int)(sizeof(chestplateRows)/sizeof(chestplateRows[0])));
    Reserve(GUI_SPRITE_EMPTY_LEGGINGS, 16, 16);
    PaintEmptySlotIcon(leggingsRows, (int)(sizeof(leggingsRows)/sizeof(leggingsRows[0])));
    Reserve(GUI_SPRITE_EMPTY_BOOTS, 16, 16);
    PaintEmptySlotIcon(bootsRows, (int)(sizeof(bootsRows)/sizeof(bootsRows[0])));
    Reserve(GUI_SPRITE_EMPTY_SHIELD, 16, 16);
    PaintEmptySlotIcon(shieldRows, (int)(sizeof(shieldRows)/sizeof(shieldRows[0])));
    Reserve(GUI_SPRITE_DESTROY_SLOT, 18, 18);
    PaintDestroySlot();

    Reserve(GUI_SPRITE_BOOK_TAB, 35, 27);
    PaintBookTab(false);
    Reserve(GUI_SPRITE_BOOK_TAB_SELECTED, 35, 27);
    PaintBookTab(true);
    Reserve(GUI_SPRITE_BOOK_SLOT_CRAFTABLE, 25, 25);
    PaintRecipeSlot(0xDBDBDB, 0xA0A0A0, 0x5B5B5B);
    Reserve(GUI_SPRITE_BOOK_SLOT_UNCRAFTABLE, 25, 25);
    PaintRecipeSlot(0xE7A5A5, 0xB45E5E, 0x6B2B2B);
    Reserve(GUI_SPRITE_BOOK_SLOT_MANY_CRAFTABLE, 25, 25);
    PaintRecipeSlotStack(0xDBDBDB, 0xA0A0A0, 0x5B5B5B);
    Reserve(GUI_SPRITE_BOOK_SLOT_MANY_UNCRAFTABLE, 25, 25);
    PaintRecipeSlotStack(0xE7A5A5, 0xB45E5E, 0x6B2B2B);
    Reserve(GUI_SPRITE_BOOK_OVERLAY_CRAFTABLE, 24, 24);
    PaintOverlayButton(0xDBDBDB, 0xA0A0A0, 0x5B5B5B, false);
    Reserve(GUI_SPRITE_BOOK_OVERLAY_CRAFTABLE_HIGHLIGHTED, 24, 24);
    PaintOverlayButton(0xE8E8E8, 0xB8B8B8, 0x6B6B6B, true);
    Reserve(GUI_SPRITE_BOOK_OVERLAY_UNCRAFTABLE, 24, 24);
    PaintOverlayButton(0xE7A5A5, 0xB45E5E, 0x6B2B2B, false);
    Reserve(GUI_SPRITE_BOOK_OVERLAY_UNCRAFTABLE_HIGHLIGHTED, 24, 24);
    PaintOverlayButton(0xF0BDBD, 0xC87474, 0x7B3B3B, true);
    Reserve(GUI_SPRITE_BOOK_FILTER_ALL, 26, 16);
    PaintFilterButton(false, false);
    Reserve(GUI_SPRITE_BOOK_FILTER_ALL_HIGHLIGHTED, 26, 16);
    PaintFilterButton(false, true);
    Reserve(GUI_SPRITE_BOOK_FILTER_CRAFTABLE, 26, 16);
    PaintFilterButton(true, false);
    Reserve(GUI_SPRITE_BOOK_FILTER_CRAFTABLE_HIGHLIGHTED, 26, 16);
    PaintFilterButton(true, true);
    Reserve(GUI_SPRITE_BOOK_PAGE_FORWARD, 12, 17);
    PaintPageArrow(true, false);
    Reserve(GUI_SPRITE_BOOK_PAGE_FORWARD_HIGHLIGHTED, 12, 17);
    PaintPageArrow(true, true);
    Reserve(GUI_SPRITE_BOOK_PAGE_BACKWARD, 12, 17);
    PaintPageArrow(false, false);
    Reserve(GUI_SPRITE_BOOK_PAGE_BACKWARD_HIGHLIGHTED, 12, 17);
    PaintPageArrow(false, true);
}

static void GuiEnsure(void)
{
    Image image = { 0 };

    if (guiReady) return;
    guiReady = true;

    image = GenImageColor(ATLAS_WIDTH, ATLAS_HEIGHT, BLANK);
    canvas = (Color *)image.data;
    packX = 0;
    packY = 0;
    packRow = 0;
    PaintAtlas();
    if (packY + packRow > ATLAS_HEIGHT) TraceLog(LOG_WARNING, "GUI: sprite atlas overflows %dx%d", ATLAS_WIDTH, ATLAS_HEIGHT);
    canvas = NULL;
    atlas = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(atlas, TEXTURE_FILTER_POINT);

    chestTexture = LoadTexture("resources/textures/entity/chest/normal.png");
    if (chestTexture.id != 0) SetTextureFilter(chestTexture, TEXTURE_FILTER_POINT);
    maskShader = LoadShaderFromMemory(NULL, maskCode);
}

void GuiUnload(void)
{
    if (!guiReady) return;
    if (atlas.id != 0) UnloadTexture(atlas);
    if (chestTexture.id != 0) UnloadTexture(chestTexture);
    if (maskShader.id != 0) UnloadShader(maskShader);
    atlas = (Texture2D){ 0 };
    chestTexture = (Texture2D){ 0 };
    maskShader = (Shader){ 0 };
    guiReady = false;
}

//----------------------------------------------------------------------------------
// Scale and clipping
//----------------------------------------------------------------------------------
int GuiScale(void)
{
    return MenuScale();
}

int GuiWidth(void)
{
    int scale = GuiScale();

    return (GetScreenWidth() + scale - 1)/scale;
}

int GuiHeight(void)
{
    int scale = GuiScale();

    return (GetScreenHeight() + scale - 1)/scale;
}

Vector2 GuiMouse(void)
{
    Vector2 mouse = GetMousePosition();
    int scale = GuiScale();

    return (Vector2){ (float)((int)mouse.x/scale), (float)((int)mouse.y/scale) };
}

void GuiBegin(void)
{
    float scale = (float)GuiScale();

    GuiEnsure();
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();
    rlPushMatrix();
    rlScalef(scale, scale, 1.0f);
}

void GuiEnd(void)
{
    rlDrawRenderBatchActive();
    rlPopMatrix();
    rlEnableBackfaceCulling();
}

void GuiScissor(int x, int y, int w, int h)
{
    int scale = GuiScale();

    BeginScissorMode(x*scale, y*scale, w*scale, h*scale);
}

void GuiScissorEnd(void)
{
    EndScissorMode();
}

//----------------------------------------------------------------------------------
// Sprites and fills
//----------------------------------------------------------------------------------
int GuiSpriteWidth(GuiSprite sprite)
{
    GuiEnsure();
    return sprites[sprite].w;
}

int GuiSpriteHeight(GuiSprite sprite)
{
    GuiEnsure();
    return sprites[sprite].h;
}

void GuiDrawSpriteRegion(GuiSprite sprite, int sx, int sy, int w, int h, int x, int y)
{
    SpriteRect rect = { 0 };

    GuiEnsure();
    if ((w <= 0) || (h <= 0)) return;
    rect = sprites[sprite];
    DrawTexturePro(atlas, (Rectangle){ (float)(rect.x + sx), (float)(rect.y + sy), (float)w, (float)h },
        (Rectangle){ (float)x, (float)y, (float)w, (float)h }, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

void GuiDrawSprite(GuiSprite sprite, int x, int y)
{
    GuiEnsure();
    GuiDrawSpriteRegion(sprite, 0, 0, sprites[sprite].w, sprites[sprite].h, x, y);
}

void GuiDrawSpriteTinted(GuiSprite sprite, int x, int y, Color tint)
{
    SpriteRect rect = { 0 };

    GuiEnsure();
    rect = sprites[sprite];
    DrawTexturePro(atlas, (Rectangle){ (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h },
        (Rectangle){ (float)x, (float)y, (float)rect.w, (float)rect.h }, (Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

void GuiFill(int x0, int y0, int x1, int y1, Color color)
{
    if ((x1 <= x0) || (y1 <= y0)) return;
    DrawRectangle(x0, y0, x1 - x0, y1 - y0, color);
}

void GuiFillGradient(int x0, int y0, int x1, int y1, Color top, Color bottom)
{
    if ((x1 <= x0) || (y1 <= y0)) return;
    DrawRectangleGradientV(x0, y0, x1 - x0, y1 - y0, top, bottom);
}

void GuiDrawPanel(int x, int y, int w, int h)
{
    Color black = Rgb(0x000000);
    Color white = Rgb(0xFFFFFF);
    Color light = Rgb(0xC6C6C6);
    Color shadow = Rgb(0x555555);

    GuiFill(x + 2, y, x + w - 3, y + 1, black);
    GuiFill(x + 1, y + 1, x + 2, y + 2, black);
    GuiFill(x + 2, y + 1, x + w - 3, y + 2, white);
    GuiFill(x + w - 3, y + 1, x + w - 2, y + 2, black);
    GuiFill(x, y + 2, x + 1, y + h - 3, black);
    GuiFill(x + 1, y + 2, x + w - 3, y + 3, white);
    GuiFill(x + w - 3, y + 2, x + w - 2, y + 3, light);
    GuiFill(x + w - 2, y + 2, x + w - 1, y + 3, black);
    GuiFill(x + 1, y + 3, x + 3, y + h - 3, white);
    GuiFill(x + 3, y + 3, x + 4, y + 4, white);
    GuiFill(x + 4, y + 3, x + w - 3, y + 4, light);
    GuiFill(x + 3, y + 4, x + w - 3, y + h - 4, light);
    GuiFill(x + 3, y + h - 4, x + w - 4, y + h - 3, light);
    GuiFill(x + w - 4, y + h - 4, x + w - 3, y + h - 3, shadow);
    GuiFill(x + w - 3, y + 3, x + w - 1, y + h - 3, shadow);
    GuiFill(x + w - 1, y + 3, x + w, y + h - 2, black);
    GuiFill(x + 1, y + h - 3, x + 2, y + h - 2, black);
    GuiFill(x + 2, y + h - 3, x + 3, y + h - 2, light);
    GuiFill(x + 3, y + h - 3, x + w - 1, y + h - 2, shadow);
    GuiFill(x + 2, y + h - 2, x + 3, y + h - 1, black);
    GuiFill(x + 3, y + h - 2, x + w - 2, y + h - 1, shadow);
    GuiFill(x + w - 2, y + h - 2, x + w - 1, y + h - 1, black);
    GuiFill(x + 3, y + h - 1, x + w - 2, y + h, black);
}

void GuiDrawInset(int x, int y, int w, int h, Color fill)
{
    GuiFill(x, y, x + w - 1, y + 1, Rgb(0x373737));
    GuiFill(x + w - 1, y, x + w, y + 1, Rgb(0x8B8B8B));
    GuiFill(x, y + 1, x + 1, y + h - 1, Rgb(0x373737));
    GuiFill(x + 1, y + 1, x + w - 1, y + h - 1, fill);
    GuiFill(x + w - 1, y + 1, x + w, y + h - 1, Rgb(0xFFFFFF));
    GuiFill(x, y + h - 1, x + 1, y + h, Rgb(0x8B8B8B));
    GuiFill(x + 1, y + h - 1, x + w, y + h, Rgb(0xFFFFFF));
}

//----------------------------------------------------------------------------------
// Text
//----------------------------------------------------------------------------------
int GuiTextWidth(const char *text)
{
    if ((text == NULL) || (text[0] == '\0')) return 0;
    return (int)MeasureTextEx(GetMenuFont(), text, 8.0f, 0.0f).x;
}

Color GuiShadowColor(Color color)
{
    return (Color){ (unsigned char)((color.r & 0xFC)/4), (unsigned char)((color.g & 0xFC)/4), (unsigned char)((color.b & 0xFC)/4), color.a };
}

void GuiDrawText(const char *text, int x, int y, Color color, bool shadow)
{
    Font font = GetMenuFont();

    if ((text == NULL) || (text[0] == '\0')) return;
    if (shadow) DrawTextEx(font, text, (Vector2){ (float)(x + 1), (float)(y + 1) }, 8.0f, 0.0f, GuiShadowColor(color));
    DrawTextEx(font, text, (Vector2){ (float)x, (float)y }, 8.0f, 0.0f, color);
}

// Italic glyphs lean a quarter pixel per row: the top row sits one pixel right
// of the upright glyph and the bottom edge one pixel left. Advances are unchanged.
static void DrawItalicRun(const char *text, float x, float y, Color color)
{
    Font font = GetMenuFont();
    float scale = 8.0f/(float)font.baseSize;
    float penX = x;

    if (font.texture.id == 0) return;
    rlSetTexture(font.texture.id);
    rlBegin(RL_QUADS);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; text[i] != '\0';)
    {
        int bytes = 0;
        int codepoint = GetCodepointNext(&text[i], &bytes);
        int glyph = GetGlyphIndex(font, codepoint);
        Rectangle rec = font.recs[glyph];
        float pad = (float)font.glyphPadding;
        float x0 = penX + ((float)font.glyphs[glyph].offsetX - pad)*scale;
        float y0 = y + ((float)font.glyphs[glyph].offsetY - pad)*scale;
        float w = (rec.width + 2.0f*pad)*scale;
        float h = (rec.height + 2.0f*pad)*scale;
        float u0 = (rec.x - pad)/(float)font.texture.width;
        float v0 = (rec.y - pad)/(float)font.texture.height;
        float u1 = (rec.x + rec.width + pad)/(float)font.texture.width;
        float v1 = (rec.y + rec.height + pad)/(float)font.texture.height;
        float topLean = 1.0f - 0.25f*(y0 - y);
        float bottomLean = 1.0f - 0.25f*(y0 + h - y);

        if ((codepoint != ' ') && (codepoint != '\t'))
        {
            rlTexCoord2f(u0, v0);
            rlVertex2f(x0 + topLean, y0);
            rlTexCoord2f(u0, v1);
            rlVertex2f(x0 + bottomLean, y0 + h);
            rlTexCoord2f(u1, v1);
            rlVertex2f(x0 + w + bottomLean, y0 + h);
            rlTexCoord2f(u1, v0);
            rlVertex2f(x0 + w + topLean, y0);
        }
        penX += ((font.glyphs[glyph].advanceX == 0)? rec.width : (float)font.glyphs[glyph].advanceX)*scale;
        i += (bytes > 0)? bytes : 1;
    }
    rlEnd();
    rlSetTexture(0);
}

void GuiDrawTextItalic(const char *text, int x, int y, Color color, bool shadow)
{
    if ((text == NULL) || (text[0] == '\0')) return;
    if (shadow) DrawItalicRun(text, (float)(x + 1), (float)(y + 1), GuiShadowColor(color));
    DrawItalicRun(text, (float)x, (float)y, color);
}

//----------------------------------------------------------------------------------
// Items
//----------------------------------------------------------------------------------
// Block items use the inventory view of a cube: 30 degrees down, turned 225
// degrees, 10 GUI pixels per block. East (+X) faces left, north (-Z) faces right.
static void IsoPoint(float ox, float oy, float x, float y, float z, float *sx, float *sy)
{
    *sx = ox + 15.0711f - 7.0711f*(x + z);
    *sy = oy + 12.3301f + 3.5355f*(x - z) - 8.6603f*y;
}

// Corners pair with (u0,v0), (u1,v0), (u1,v1), (u0,v1).
static void IsoQuad(float ox, float oy, const float corners[4][3], float u0, float v0, float u1, float v1, unsigned char shade)
{
    float sx[4] = { 0 };
    float sy[4] = { 0 };

    for (int i = 0; i < 4; i++) IsoPoint(ox, oy, corners[i][0], corners[i][1], corners[i][2], &sx[i], &sy[i]);
    rlColor4ub(shade, shade, shade, 255);
    rlTexCoord2f(u0, v0);
    rlVertex2f(sx[0], sy[0]);
    rlTexCoord2f(u0, v1);
    rlVertex2f(sx[3], sy[3]);
    rlTexCoord2f(u1, v1);
    rlVertex2f(sx[2], sy[2]);
    rlTexCoord2f(u1, v0);
    rlVertex2f(sx[1], sy[1]);
}

static const float topCorners[4][3] = { { 0, 1, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 0, 1, 1 } };
static const float eastCorners[4][3] = { { 1, 1, 1 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 1 } };
static const float northCorners[4][3] = { { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 0 }, { 1, 0, 0 } };
static const float bottomCorners[4][3] = { { 0, 0, 1 }, { 1, 0, 1 }, { 1, 0, 0 }, { 0, 0, 0 } };
static const float westCorners[4][3] = { { 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 1 }, { 0, 0, 0 } };
static const float southCorners[4][3] = { { 0, 1, 1 }, { 1, 1, 1 }, { 1, 0, 1 }, { 0, 0, 1 } };

static void BlockFace(BlockType block, int face, const float corners[4][3], float ox, float oy, unsigned char shade, float inset)
{
    float u = 0.0f;
    float v = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    GetBlockTextureUV(block, face, &u, &v, &w, &h);
    IsoQuad(ox, oy, corners, u + inset, v + inset, u + w - inset, v + h - inset, shade);
}

static void DrawBlockIcon(BlockType block, float ox, float oy)
{
    Texture2D blocks = GetTextureAtlas();
    float inset = 0.0f;

    if (blocks.id == 0) return;
    inset = 0.02f/(float)blocks.width;
    rlSetTexture(blocks.id);
    rlBegin(RL_QUADS);
    if (BlockNeedsAlphaBlending(block))
    {
        BlockFace(block, FACE_BOTTOM, bottomCorners, ox, oy, 102, inset);
        BlockFace(block, FACE_LEFT, westCorners, ox, oy, 217, inset);
        BlockFace(block, FACE_FRONT, southCorners, ox, oy, 189, inset);
    }
    BlockFace(block, FACE_TOP, topCorners, ox, oy, 255, inset);
    BlockFace(block, FACE_RIGHT, eastCorners, ox, oy, 163, inset);
    BlockFace(block, FACE_BACK, northCorners, ox, oy, 102, inset);
    rlEnd();
    rlSetTexture(0);
}

// Chest parts in sixteenths of a block, front and latch facing north. The UV
// boxes are pixel rectangles on the 64x64 chest texture.
static void ChestQuad(float ox, float oy, const float corners[4][3], int u0, int v0, int u1, int v1, unsigned char shade)
{
    float scaled[4][3] = { 0 };
    float texel = 1.0f/64.0f;
    float inset = 0.02f/64.0f;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 3; j++) scaled[i][j] = corners[i][j]/16.0f;
    }
    IsoQuad(ox, oy, (const float (*)[3])scaled, u0*texel + inset, v0*texel + inset, u1*texel - inset, v1*texel - inset, shade);
}

static void DrawChestIcon(float ox, float oy)
{
    static const float baseEast[4][3] = { { 15, 10, 15 }, { 15, 10, 1 }, { 15, 0, 1 }, { 15, 0, 15 } };
    static const float baseNorth[4][3] = { { 15, 10, 1 }, { 1, 10, 1 }, { 1, 0, 1 }, { 15, 0, 1 } };
    static const float lidTop[4][3] = { { 15, 14, 15 }, { 1, 14, 15 }, { 1, 14, 1 }, { 15, 14, 1 } };
    static const float lidEast[4][3] = { { 15, 14, 15 }, { 15, 14, 1 }, { 15, 9, 1 }, { 15, 9, 15 } };
    static const float lidNorth[4][3] = { { 15, 14, 1 }, { 1, 14, 1 }, { 1, 9, 1 }, { 15, 9, 1 } };
    static const float latchTop[4][3] = { { 9, 11, 1 }, { 7, 11, 1 }, { 7, 11, 0 }, { 9, 11, 0 } };
    static const float latchEast[4][3] = { { 9, 11, 1 }, { 9, 11, 0 }, { 9, 7, 0 }, { 9, 7, 1 } };
    static const float latchNorth[4][3] = { { 9, 11, 0 }, { 7, 11, 0 }, { 7, 7, 0 }, { 9, 7, 0 } };

    if (chestTexture.id == 0)
    {
        DrawBlockIcon(BLOCK_CHEST, ox, oy);
        return;
    }
    rlSetTexture(chestTexture.id);
    rlBegin(RL_QUADS);
    ChestQuad(ox, oy, baseEast, 0, 33, 14, 43, 163);
    ChestQuad(ox, oy, baseNorth, 14, 33, 28, 43, 102);
    ChestQuad(ox, oy, lidTop, 14, 0, 28, 14, 255);
    ChestQuad(ox, oy, lidEast, 0, 14, 14, 19, 163);
    ChestQuad(ox, oy, lidNorth, 14, 14, 28, 19, 102);
    ChestQuad(ox, oy, latchTop, 1, 0, 3, 1, 255);
    ChestQuad(ox, oy, latchEast, 0, 1, 1, 5, 163);
    ChestQuad(ox, oy, latchNorth, 1, 1, 3, 5, 102);
    rlEnd();
    rlSetTexture(0);
}

bool GuiIsFlatItem(BlockType block)
{
    return (block == BLOCK_BUCKET) || (block == BLOCK_WATER_BUCKET);
}

int GuiMaxStack(BlockType block)
{
    if (block == BLOCK_BUCKET) return 16;
    if (block == BLOCK_WATER_BUCKET) return 1;
    return 64;
}

void GuiDrawItemTexture(const char *textureName, int x, int y)
{
    Texture2D blocks = GetTextureAtlas();
    float u = 0.0f;
    float v = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    if ((blocks.id == 0) || !GetNamedTextureUV(textureName, &u, &v, &w, &h)) return;
    DrawTexturePro(blocks,
        (Rectangle){ u*(float)blocks.width, v*(float)blocks.height, w*(float)blocks.width, h*(float)blocks.height },
        (Rectangle){ (float)x, (float)y, 16.0f, 16.0f }, (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}

void GuiDrawItem(BlockType block, int x, int y)
{
    if ((block <= BLOCK_AIR) || (block >= BLOCK_COUNT)) return;
    GuiEnsure();
    if (block == BLOCK_CHEST) DrawChestIcon((float)x, (float)y);
    else if (GuiIsFlatItem(block)) GuiDrawItemTexture(GetBlockTextureName(block, FACE_TOP), x, y);
    else DrawBlockIcon(block, (float)x, (float)y);
}

void GuiDrawItemScaled(BlockType block, float x, float y, float scale)
{
    rlPushMatrix();
    rlTranslatef(x, y, 0.0f);
    rlScalef(scale, scale, 1.0f);
    GuiDrawItem(block, 0, 0);
    rlPopMatrix();
}

// The item marks the stencil where its texels are opaque, then the fill is
// drawn only there and clears the marks as it goes.
void GuiDrawItemSilhouette(BlockType block, int x, int y, Color color)
{
    if ((block <= BLOCK_AIR) || (block >= BLOCK_COUNT)) return;
    GuiEnsure();
    if (maskShader.id == 0) return;
    rlDrawRenderBatchActive();
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    BeginShaderMode(maskShader);
    GuiDrawItem(block, x, y);
    EndShaderMode();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
    GuiFill(x, y, x + 16, y + 16, color);
    rlDrawRenderBatchActive();
    glDisable(GL_STENCIL_TEST);
}

void GuiDrawItemCount(int count, int x, int y)
{
    if ((count == 1) || (count <= 0)) return;
    GuiDrawItemCountText(TextFormat("%d", count), x, y, WHITE);
}

void GuiDrawItemCountText(const char *text, int x, int y, Color color)
{
    GuiDrawText(text, x + 17 - GuiTextWidth(text), y + 9, color, true);
}

void GuiDrawItemStack(BlockType block, int count, int x, int y)
{
    if ((block == BLOCK_AIR) || (count <= 0)) return;
    GuiDrawItem(block, x, y);
    GuiDrawItemCount(count, x, y);
}

void GuiDrawSlotHighlight(int x, int y)
{
    GuiFill(x, y, x + 16, y + 16, (Color){ 255, 255, 255, 128 });
}

//----------------------------------------------------------------------------------
// Tooltip
//----------------------------------------------------------------------------------
void GuiDrawTooltip(const char **lines, const Color *colors, int lineCount, int mouseX, int mouseY)
{
    GuiDrawTooltipEx(lines, colors, NULL, lineCount, mouseX, mouseY);
}

void GuiDrawTooltipEx(const char **lines, const Color *colors, const bool *italic, int lineCount, int mouseX, int mouseY)
{
    Color background = { 16, 0, 16, 240 };
    Color borderTop = { 80, 0, 255, 80 };
    Color borderBottom = { 40, 0, 127, 80 };
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
    int lineY = 0;

    if ((lines == NULL) || (lineCount <= 0)) return;
    for (int i = 0; i < lineCount; i++)
    {
        int lineWidth = GuiTextWidth(lines[i]);

        if (lineWidth > width) width = lineWidth;
    }
    height = (lineCount == 1)? 8 : 10*lineCount;
    x = mouseX + 12;
    y = mouseY - 12;
    if (x + width > GuiWidth()) x = (mouseX + 12 - 24 - width > 4)? mouseX + 12 - 24 - width : 4;
    if (y + height + 3 > GuiHeight()) y = GuiHeight() - height - 3;

    GuiFill(x - 3, y - 4, x + width + 3, y - 3, background);
    GuiFill(x - 3, y + height + 3, x + width + 3, y + height + 4, background);
    GuiFill(x - 3, y - 3, x + width + 3, y + height + 3, background);
    GuiFill(x - 4, y - 3, x - 3, y + height + 3, background);
    GuiFill(x + width + 3, y - 3, x + width + 4, y + height + 3, background);
    GuiFillGradient(x - 3, y - 2, x - 2, y + height + 2, borderTop, borderBottom);
    GuiFillGradient(x + width + 2, y - 2, x + width + 3, y + height + 2, borderTop, borderBottom);
    GuiFill(x - 3, y - 3, x + width + 3, y - 2, borderTop);
    GuiFill(x - 3, y + height + 2, x + width + 3, y + height + 3, borderBottom);

    lineY = y;
    for (int i = 0; i < lineCount; i++)
    {
        Color color = (colors != NULL)? colors[i] : WHITE;

        if ((italic != NULL) && italic[i]) GuiDrawTextItalic(lines[i], x, lineY, color, true);
        else GuiDrawText(lines[i], x, lineY, color, true);
        lineY += (i == 0)? 12 : 10;
    }
}
