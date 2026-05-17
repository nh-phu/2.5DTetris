#include "ui.h"

#include "piece_colors.h"

#include <algorithm>
#include <cstdio>

void UI::DrawPiece(Renderer &r, const Board &board, const Pieces &pieces, int x,
                   int y, int piece, int rotation)
{
    RenderColor c = PieceRenderColorForKind(piece);

    // Small inset so blocks don't visually merge.
    static const int kPad = 1;

    int pixelsX = board.GetXPosInPixels(x);
    int pixelsY = board.GetYPosInPixels(y);

    for (int i = 0; i < PIECE_BLOCKS; i++) {
        for (int j = 0; j < PIECE_BLOCKS; j++) {
            if (pieces.GetBlockType(piece, rotation, j, i) != 0)
                r.DrawRectangle(
                    pixelsX + i * BLOCK_SIZE + kPad,
                    pixelsY + j * BLOCK_SIZE + kPad,
                    (pixelsX + i * BLOCK_SIZE) + BLOCK_SIZE - 1 - kPad,
                    (pixelsY + j * BLOCK_SIZE) + BLOCK_SIZE - 1 - kPad, c);
        }
    }
}

void UI::DrawBoard(Renderer &r, const Board &board, const Game &game,
                   int screenHeight)
{
    int x1 = BOARD_POSITION - (BLOCK_SIZE * (BOARD_WIDTH / 2)) - 1;
    int x2 = BOARD_POSITION + (BLOCK_SIZE * (BOARD_WIDTH / 2));
    int y = screenHeight - (BLOCK_SIZE * BOARD_HEIGHT);

    r.DrawRectangle(x1 - BOARD_LINE_WIDTH, y, x1, screenHeight - 1,
                    RenderColor::Blue);
    r.DrawRectangle(x2, y, x2 + BOARD_LINE_WIDTH, screenHeight - 1,
                    RenderColor::Blue);

    x1 += 1;
    static const int kPad = 1;
    for (int i = 0; i < BOARD_WIDTH; i++) {
        for (int j = 0; j < BOARD_HEIGHT; j++) {
            int kind = board.BlockKind(i, j);
            if (kind >= 0)
                r.DrawRectangle(x1 + i * BLOCK_SIZE + kPad,
                                y + j * BLOCK_SIZE + kPad,
                                (x1 + i * BLOCK_SIZE) + BLOCK_SIZE - 1 - kPad,
                                (y + j * BLOCK_SIZE) + BLOCK_SIZE - 1 - kPad,
                                PieceRenderColorForKind(kind));
        }
    }
}

void UI::Draw(Renderer &r, const Board &board, const Pieces &pieces,
              const Game &game, int screenHeight)
{
    DrawBoard(r, board, game, screenHeight);
    DrawPiece(r, board, pieces, game.ActiveX(), game.ActiveY(),
              game.ActivePiece(), game.ActiveRotation());

    DrawHud(r, board, pieces, game);
}

void UI::DrawHud(Renderer &r, const Board &board, const Pieces &pieces,
                 const Game &game)
{
    const int screenW = r.GetScreenWidth();

    const int kMargin = 8;
    const int kGap = 10;

    // HUD typography: change only this.
    const int hudLabelPx = 16;
    // Keep values proportional and snapped to even pixels for crisp scaling.
    const int hudValuePx = ((hudLabelPx * 3 / 2) & ~1);

    const int padX = std::max(6, hudLabelPx / 2);
    const int topPad = std::max(4, hudLabelPx / 3);
    const int bottomPad = topPad;
    const int labelGap = std::max(4, hudLabelPx / 2);

    // NEXT preview sizing is derived from label size (not tied to board blocks).
    // This keeps NEXT/FACE panels compact while preserving readability.
    int previewBlockPx = std::max(8, (hudLabelPx * 3) / 4);
    previewBlockPx = (previewBlockPx & ~1); // even pixels for crisp scaling

    // Compute occupied bounds for the NEXT piece so we size by actual content.
    int minX = PIECE_BLOCKS, minY = PIECE_BLOCKS;
    int maxX = -1, maxY = -1;
    for (int i = 0; i < PIECE_BLOCKS; i++) {
        for (int j = 0; j < PIECE_BLOCKS; j++) {
            int t = pieces.GetBlockType(game.NextPiece(), game.NextRotation(), j, i);
            if (t == 0)
                continue;
            minX = std::min(minX, i);
            minY = std::min(minY, j);
            maxX = std::max(maxX, i);
            maxY = std::max(maxY, j);
        }
    }

    const int occWBlocks = (maxX >= 0) ? (maxX - minX + 1) : 0;
    const int occHBlocks = (maxY >= 0) ? (maxY - minY + 1) : 0;
    const int nextPreviewW = occWBlocks * previewBlockPx;
    const int nextPreviewH = occHBlocks * previewBlockPx;

    // Content-based sizing:
    // - SCORE is stable-width
    // - FACE and NEXT are forced to be identical size
    // - SCORE width equals (FACE + gap + NEXT)
    const int availW = screenW - (kMargin * 2);

    const int scoreNeedW =
        r.MeasureTextWidth("999999", hudValuePx) + (padX * 2);
    const int faceNeedW =
        std::max(r.MeasureTextWidth("FACE", hudLabelPx),
                 r.MeasureTextWidth("4/4", hudValuePx)) +
        (padX * 2);
    const int nextNeedW =
        std::max(r.MeasureTextWidth("NEXT", hudLabelPx), nextPreviewW) +
        (padX * 2);

    const int faceNeedH = topPad + hudLabelPx + labelGap + hudValuePx + bottomPad;
    const int nextNeedH =
        topPad + hudLabelPx + labelGap + nextPreviewH + bottomPad;

    // Small panels share size (width + height).
    int smallW = std::max(faceNeedW, nextNeedW);
    int smallH = std::max(faceNeedH, nextNeedH);

    // Ensure SCORE width is stable and equals (FACE + gap + NEXT).
    // Choose smallW large enough for both content and score digits.
    if (kGap < availW) {
        int fromScore = (scoreNeedW > kGap) ? ((scoreNeedW - kGap + 1) / 2) : 0;
        smallW = std::max(smallW, fromScore);
    }

    if (kGap < availW) {
        int maxSmallW = (availW - kGap) / 2;
        if (smallW > maxSmallW) {
            // Screen too narrow: clamp widths, then shrink NEXT preview to fit.
            smallW = std::max(0, maxSmallW);

            const int maxPreviewWInner = std::max(0, smallW - (padX * 2));
            int fitBlock = (occWBlocks > 0) ? (maxPreviewWInner / occWBlocks) : 0;
            fitBlock = std::max(6, fitBlock);
            fitBlock = (fitBlock & ~1);
            previewBlockPx = std::min(previewBlockPx, fitBlock);
        }
    }

    const int faceW = smallW;
    const int nextW = smallW;
    const int faceH = smallH;
    const int nextH = smallH;

    int scoreW = kGap + (2 * smallW);
    scoreW = std::min(scoreW, availW);

    // SCORE is slightly taller so it doesn't look cramped.
    const int scoreBaseH = topPad + hudLabelPx + labelGap + hudValuePx + bottomPad;
    const int scoreExtraH = std::max(4, hudLabelPx / 2);
    const int scoreH = std::max(scoreBaseH + scoreExtraH, smallH + 2);

    // Layout:
    // Row 1: SCORE (top-right)
    // Row 2: FACE then NEXT (same size)
    const int x2 = screenW - kMargin;
    const int scoreX2 = x2;
    const int scoreX1 = scoreX2 - scoreW;
    const int scoreY1 = kMargin;
    const int scoreY2 = scoreY1 + scoreH;

    const int row2Y1 = scoreY2 + kGap;
    const int row2Y2 = row2Y1 + nextH;

    const int faceX1 = scoreX1;
    const int faceX2 = faceX1 + faceW;
    const int nextX1 = faceX2 + kGap;
    const int nextX2 = nextX1 + nextW;

    auto DrawPanel = [&](int x1, int y1, int x2, int y2) {
        r.DrawRectangle(x1, y1, x2, y2, RenderColor::DarkGray);
        r.DrawRectangle(x1, y1, x2, y1 + 1, RenderColor::LightGray);
        r.DrawRectangle(x1, y2 - 1, x2, y2, RenderColor::LightGray);
        r.DrawRectangle(x1, y1, x1 + 1, y2, RenderColor::LightGray);
        r.DrawRectangle(x2 - 1, y1, x2, y2, RenderColor::LightGray);
    };

    DrawPanel(scoreX1, scoreY1, scoreX2, scoreY2);
    DrawPanel(faceX1, row2Y1, faceX2, row2Y2);
    DrawPanel(nextX1, row2Y1, nextX2, row2Y2);

    const char *scoreLabel = "SCORE";
    int scoreLabelW = r.MeasureTextWidth(scoreLabel, hudLabelPx);
    int scoreLabelX = scoreX1 + (scoreW - scoreLabelW) / 2;
    int scoreLabelY = scoreY1 + topPad;
    r.DrawText(scoreLabelX, scoreLabelY, scoreLabel, hudLabelPx,
               RenderColor::White);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d", game.Score());
    int scoreValueW = r.MeasureTextWidth(buf, hudValuePx);
    int scoreValueX = scoreX1 + (scoreW - scoreValueW) / 2;
    int scoreValueY = scoreLabelY + hudLabelPx + labelGap;
    r.DrawText(scoreValueX, scoreValueY, buf, hudValuePx,
               RenderColor::Yellow);

    const char *nextLabel = "NEXT";
    int nextLabelW = r.MeasureTextWidth(nextLabel, hudLabelPx);
    int nextLabelX = nextX1 + (nextW - nextLabelW) / 2;
    int nextLabelY = row2Y1 + topPad;
    r.DrawText(nextLabelX, nextLabelY, nextLabel, hudLabelPx,
               RenderColor::White);

    const int boxInnerX1 = nextX1 + padX;
    const int boxInnerX2 = nextX2 - padX;
    const int boxInnerY1 = nextLabelY + hudLabelPx + labelGap;
    const int boxInnerY2 = row2Y2 - bottomPad;

    const int previewCenterX = (boxInnerX1 + boxInnerX2) / 2;
    const int previewCenterY = (boxInnerY1 + boxInnerY2) / 2;

    // Center the NEXT piece by occupied bounds (not the 5x5 grid).
    int originX = previewCenterX;
    int originY = previewCenterY;
    if (occWBlocks > 0 && occHBlocks > 0) {
        const int occW = occWBlocks * previewBlockPx;
        const int occH = occHBlocks * previewBlockPx;
        originX = previewCenterX - (occW / 2) - (minX * previewBlockPx);
        originY = previewCenterY - (occH / 2) - (minY * previewBlockPx);
    }

    RenderColor nextColor = PieceRenderColorForKind(game.NextPiece());
    for (int i = 0; i < PIECE_BLOCKS; i++) {
        for (int j = 0; j < PIECE_BLOCKS; j++) {
            int t = pieces.GetBlockType(game.NextPiece(), game.NextRotation(), j, i);
            if (t == 0)
                continue;

            const int xA = originX + i * previewBlockPx;
            const int yA = originY + j * previewBlockPx;
            r.DrawRectangle(xA + 1, yA + 1, xA + previewBlockPx - 2,
                            yA + previewBlockPx - 2, nextColor);
        }
    }

    const char *faceLabel = "FACE";
    int faceLabelW = r.MeasureTextWidth(faceLabel, hudLabelPx);
    int faceLabelX = faceX1 + (faceW - faceLabelW) / 2;
    int faceLabelY = row2Y1 + topPad;
    r.DrawText(faceLabelX, faceLabelY, faceLabel, hudLabelPx,
               RenderColor::White);

    std::snprintf(buf, sizeof(buf), "%d/4", board.ActiveFace() + 1);
    int faceValueW = r.MeasureTextWidth(buf, hudValuePx);
    int faceValueX = faceX1 + (faceW - faceValueW) / 2;
    int faceValueY = faceLabelY + hudLabelPx + labelGap;
    r.DrawText(faceValueX, faceValueY, buf, hudValuePx,
               RenderColor::White);
}
