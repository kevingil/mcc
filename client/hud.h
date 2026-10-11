#ifndef HUD_H
#define HUD_H

#include "voxel_types.h"
#include "voxel_world.h"

// The in-game overlay: hotbar, crosshair, status bars, the held item's name,
// the action bar line, and chat. Timers advance at 20 ticks per second.
void HudReset(void);
void HudUpdate(Player *player, VoxelWorld *world);
void HudDraw(const Player *player, bool debug);
void HudDrawMessages(void);

void HudChat(const char *text, Color color);
void HudClearChat(void);
void HudActionBar(const char *text);

#endif // HUD_H
