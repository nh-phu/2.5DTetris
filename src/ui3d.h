#ifndef _UI3D_
#define _UI3D_

#include "board.h"
#include "game.h"
#include "pieces.h"

namespace UI3D
{
void DrawActiveFace(const Board &board, const Pieces &pieces, const Game &game);
void DrawRotate(const Board &board, const Game &game, int fromFace,
                float angleDeg);
void Shutdown();
}

#endif
