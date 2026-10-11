#include "player_model.h"
#include "gui.h"

#include "rlgl.h"

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
    #include <GLES2/gl2.h>
#else
    #include <GL/gl.h>
#endif

#include <math.h>
#include <stddef.h>

#define SKIN_SIZE 64.0f
#define UV_INSET 0.01f

enum {
    PART_HEAD = 0,
    PART_BODY,
    PART_RIGHT_ARM,
    PART_LEFT_ARM,
    PART_RIGHT_LEG,
    PART_LEFT_LEG,
    PART_COUNT
};

// Pivot in model pixels (y points down) and rotations in radians, applied X, then Y, then Z.
typedef struct {
    float x;
    float y;
    float z;
    float xRot;
    float yRot;
    float zRot;
} PartPose;

// Box corner and size in model pixels, the skin texel where its unwrap starts,
// and how far the second skin layer stands off the first.
typedef struct {
    int part;
    float x;
    float y;
    float z;
    float w;
    float h;
    float d;
    int u;
    int v;
    float grow;
} Cube;

typedef struct {
    float feetX;
    float feetY;
    float scale;
    float bodyYaw;
    float tilt;
} ModelView;

static const Cube cubes[] = {
    { PART_HEAD, -4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0, 0, 0.0f },
    { PART_BODY, -4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 16, 16, 0.0f },
    { PART_RIGHT_ARM, -3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 40, 16, 0.0f },
    { PART_LEFT_ARM, -1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 32, 48, 0.0f },
    { PART_RIGHT_LEG, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0, 16, 0.0f },
    { PART_LEFT_LEG, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 16, 48, 0.0f },
    { PART_HEAD, -4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 32, 0, 0.5f },
    { PART_BODY, -4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 16, 32, 0.25f },
    { PART_RIGHT_ARM, -3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 40, 32, 0.25f },
    { PART_LEFT_ARM, -1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 48, 48, 0.25f },
    { PART_RIGHT_LEG, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0, 32, 0.25f },
    { PART_LEFT_LEG, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0, 48, 0.25f }
};

#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
static const char *cutoutCode =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(texture0, fragTexCoord);\n"
    "    if (texel.a < 0.1) discard;\n"
    "    gl_FragColor = texel*colDiffuse*fragColor;\n"
    "}\n";
#else
static const char *cutoutCode =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture(texture0, fragTexCoord);\n"
    "    if (texel.a < 0.1) discard;\n"
    "    finalColor = texel*colDiffuse*fragColor;\n"
    "}\n";
#endif

static bool modelReady = false;
static Texture2D skin = { 0 };
static Shader cutout = { 0 };

static void ModelEnsure(void)
{
    if (modelReady) return;
    modelReady = true;
    skin = LoadTexture("resources/textures/entity/steve.png");
    if (skin.id != 0) SetTextureFilter(skin, TEXTURE_FILTER_POINT);
    cutout = LoadShaderFromMemory(NULL, cutoutCode);
}

void PlayerModelUnload(void)
{
    if (!modelReady) return;
    if (skin.id != 0) UnloadTexture(skin);
    if (cutout.id != 0) UnloadShader(cutout);
    skin = (Texture2D){ 0 };
    cutout = (Shader){ 0 };
    modelReady = false;
}

static Vector3 RotX(Vector3 p, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    return (Vector3){ p.x, p.y*c - p.z*s, p.y*s + p.z*c };
}

static Vector3 RotY(Vector3 p, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    return (Vector3){ p.x*c + p.z*s, p.y, -p.x*s + p.z*c };
}

static Vector3 RotZ(Vector3 p, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    return (Vector3){ p.x*c - p.y*s, p.x*s + p.y*c, p.z };
}

static Vector3 PartRotate(Vector3 p, const PartPose *pose)
{
    return RotZ(RotY(RotX(p, pose->xRot), pose->yRot), pose->zRot);
}

// The entity stands with its feet at the origin, facing -Z, and the screen
// turns it about its feet to face the viewer.
static Vector3 EntityToView(Vector3 p, const ModelView *view)
{
    p.x = -p.x;
    p.y = -p.y;
    p = RotY(p, view->bodyYaw);
    p = RotX(p, view->tilt);
    return RotZ(p, PI);
}

static Vector3 ModelToGui(Vector3 corner, const PartPose *pose, const ModelView *view)
{
    Vector3 p = PartRotate(corner, pose);

    p = (Vector3){ (p.x + pose->x)/16.0f, (p.y + pose->y)/16.0f - 1.501f, (p.z + pose->z)/16.0f };
    p = EntityToView(Vector3Scale(p, 0.9375f), view);
    return (Vector3){ view->feetX + view->scale*p.x, view->feetY + view->scale*p.y, 50.0f - view->scale*p.z };
}

// Two fixed lights from above, 60% diffuse over 40% ambient: the face gets
// about 0.82, the top of the head full light, the sides about half.
static unsigned char FaceShade(Vector3 normal, const PartPose *pose, const ModelView *view)
{
    Vector3 lightA = Vector3Normalize((Vector3){ 0.2f, -1.0f, -1.0f });
    Vector3 lightB = Vector3Normalize((Vector3){ -0.2f, -1.0f, 0.0f });
    Vector3 n = EntityToView(PartRotate(normal, pose), view);
    float light = (fmaxf(0.0f, Vector3DotProduct(lightA, n)) + fmaxf(0.0f, Vector3DotProduct(lightB, n)))*0.6f + 0.4f;

    if (light > 1.0f) light = 1.0f;
    return (unsigned char)(light*255.0f + 0.5f);
}

static float Toward(float from, float to)
{
    return (to > from)? from + UV_INSET : from - UV_INSET;
}

// Corners take the texture corners (u2,v1), (u1,v1), (u1,v2), (u2,v2) in that order.
static void CubeFace(const Vector3 corners[4], float u1, float v1, float u2, float v2, Vector3 normal,
    const PartPose *pose, const ModelView *view)
{
    float us[4] = { Toward(u2, u1), Toward(u1, u2), Toward(u1, u2), Toward(u2, u1) };
    float vs[4] = { Toward(v1, v2), Toward(v1, v2), Toward(v2, v1), Toward(v2, v1) };
    unsigned char shade = FaceShade(normal, pose, view);

    rlColor4ub(shade, shade, shade, 255);
    for (int i = 0; i < 4; i++)
    {
        Vector3 p = ModelToGui(corners[i], pose, view);

        rlTexCoord2f(us[i]/SKIN_SIZE, vs[i]/SKIN_SIZE);
        // rlgl's 2D projection keeps z in [-1, 0], nearer is larger.
        rlVertex3f(p.x, p.y, (p.z - 50.0f)/200.0f - 0.5f);
    }
}

static void DrawModelCube(const Cube *cube, const PartPose *pose, const ModelView *view)
{
    float x0 = cube->x - cube->grow;
    float y0 = cube->y - cube->grow;
    float z0 = cube->z - cube->grow;
    float x1 = cube->x + cube->w + cube->grow;
    float y1 = cube->y + cube->h + cube->grow;
    float z1 = cube->z + cube->d + cube->grow;
    float u = (float)cube->u;
    float v = (float)cube->v;
    float w = cube->w;
    float h = cube->h;
    float d = cube->d;
    Vector3 c[8] = {
        { x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, z0 }, { x0, y1, z0 },
        { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 }, { x0, y1, z1 }
    };
    Vector3 down[4] = { c[5], c[4], c[0], c[1] };
    Vector3 up[4] = { c[2], c[3], c[7], c[6] };
    Vector3 west[4] = { c[0], c[4], c[7], c[3] };
    Vector3 north[4] = { c[1], c[0], c[3], c[2] };
    Vector3 east[4] = { c[5], c[1], c[2], c[6] };
    Vector3 south[4] = { c[4], c[5], c[6], c[7] };

    CubeFace(down, u + d, v, u + d + w, v + d, (Vector3){ 0.0f, -1.0f, 0.0f }, pose, view);
    CubeFace(up, u + d + w, v + d, u + d + 2.0f*w, v, (Vector3){ 0.0f, 1.0f, 0.0f }, pose, view);
    CubeFace(west, u, v + d, u + d, v + d + h, (Vector3){ -1.0f, 0.0f, 0.0f }, pose, view);
    CubeFace(north, u + d, v + d, u + d + w, v + d + h, (Vector3){ 0.0f, 0.0f, -1.0f }, pose, view);
    CubeFace(east, u + d + w, v + d, u + 2.0f*d + w, v + d + h, (Vector3){ 1.0f, 0.0f, 0.0f }, pose, view);
    CubeFace(south, u + 2.0f*d + w, v + d, u + 2.0f*d + 2.0f*w, v + d + h, (Vector3){ 0.0f, 0.0f, 1.0f }, pose, view);
}

// Standing still: the head looks along, and both arms sway a little with time.
static void PoseModel(PartPose *poses, float headYaw, float headPitch, float age)
{
    float sway = cosf(age*0.09f)*0.05f + 0.05f;
    float swing = sinf(age*0.067f)*0.05f;

    poses[PART_HEAD] = (PartPose){ 0.0f, 0.0f, 0.0f, headPitch, headYaw, 0.0f };
    poses[PART_BODY] = (PartPose){ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    poses[PART_RIGHT_ARM] = (PartPose){ -5.0f, 2.0f, 0.0f, swing, 0.0f, sway };
    poses[PART_LEFT_ARM] = (PartPose){ 5.0f, 2.0f, 0.0f, -swing, 0.0f, -sway };
    poses[PART_RIGHT_LEG] = (PartPose){ -1.9f, 12.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    poses[PART_LEFT_LEG] = (PartPose){ 1.9f, 12.0f, 0.0f, 0.0f, 0.0f, 0.0f };
}

void PlayerModelDraw(int x, int y, int scale, float lookX, float lookY, int clipX0, int clipY0, int clipX1, int clipY1)
{
    PartPose poses[PART_COUNT] = { 0 };
    ModelView view = { 0 };
    float turn = 0.0f;
    float lift = 0.0f;

    ModelEnsure();
    if (skin.id == 0) return;
    view.feetX = (float)x;
    view.feetY = (float)y;
    view.scale = (float)scale;
    turn = atanf(lookX/40.0f);
    lift = atanf(lookY/40.0f);
    view.bodyYaw = -turn*20.0f*DEG2RAD;
    view.tilt = lift*20.0f*DEG2RAD;
    PoseModel(poses, turn*20.0f*DEG2RAD, -lift*20.0f*DEG2RAD, (float)(GetTime()*20.0));

    GuiScissor(clipX0, clipY0, clipX1 - clipX0, clipY1 - clipY0);
    rlEnableDepthMask();
    glClear(GL_DEPTH_BUFFER_BIT);
    rlEnableDepthTest();
    if (cutout.id != 0) BeginShaderMode(cutout);
    rlSetTexture(skin.id);
    rlBegin(RL_QUADS);
    for (int i = 0; i < (int)(sizeof(cubes)/sizeof(cubes[0])); i++) DrawModelCube(&cubes[i], &poses[cubes[i].part], &view);
    rlEnd();
    rlSetTexture(0);
    if (cutout.id != 0) EndShaderMode();
    rlDrawRenderBatchActive();
    rlDisableDepthTest();
    GuiScissorEnd();
}
