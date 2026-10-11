#ifndef PLAYER_MODEL_H
#define PLAYER_MODEL_H

#include "voxel_types.h"

// The player skin as a boxed model, drawn in GUI pixels between GuiBegin()
// and GuiEnd(). The feet stand at (x, y) and a block is scale GUI pixels
// tall. lookX and lookY are how far the point the model watches lies left of
// and above it; the body and head turn toward it the way the inventory turns
// them toward the mouse. Nothing is drawn outside (clipX0,clipY0)-(clipX1,clipY1).
void PlayerModelDraw(int x, int y, int scale, float lookX, float lookY, int clipX0, int clipY0, int clipX1, int clipY1);
void PlayerModelUnload(void);

#endif // PLAYER_MODEL_H
