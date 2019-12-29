﻿/*
    Copyright (c) 2019 epsimatt

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "core.h"
#include "level.h"
#include "util.h"
#include "vec.h"

#define ISTR_SZ 16

static const float FALLING_SPEED = 2.4f;

static Texture2D *tx_list[TXL_LEN] = { 
    &tx_blocks,
    &tx_border,
    &tx_clicked,
    &tx_playfield
};

static const char *txf_list[TXL_LEN] = {
    "res/images/blocks.png",
    "res/images/border.png",
    "res/images/clicked.png",
    "res/images/playfield.png",
};

static Block playfield[PF_HEIGHT][PF_WIDTH];

static Block *adjacent_blocks[4];
static Block *selected_block;

static char current_score_str[ISTR_SZ];
static char highest_score_str[ISTR_SZ];
static char elapsed_time_str[ISTR_SZ];

static int result;

/* 배경 화면을 그린다. */
static void DrawBackground(void);

/* 전경 화면을 그린다. */
static void DrawForeground(void);

/* 블록이 놓이는 공간을 그린다. */
static void DrawPlayfield(void);

/* 주어진 블록과 인접한 모든 블록을 찾고 삭제한다. */
static void FindMatches(Block *block);

/* 주어진 좌표에 있는 블록의 메모리 주소를 구한다. */
static Block *GetBlock(int px, int py);

/* 마우스 커서가 주어진 블록의 어느 방향에 있는지 구한다. */
static int GetRelativeCursorPosition(Block *block);

/* 마우스 이벤트를 처리한다. */
static void HandleMouseEvents(void);

/* 마우스 커서가 주어진 블록을 가리키고 있는지 확인한다. */
static bool IsBlockSelected(Block *block);

/* 블록이 사라지는 애니메이션을 재생한다. */
static void PlayBlockAnimation(Block *block);

/* 주어진 좌표에 블록을 놓는다. */
static void SetBlock(int px, int py, Block *block);

/* 블록의 위치를 업데이트한다. */
static void UpdateBlockPosition(int px, int py);

/* 배경 화면을 그린다. */
static void DrawBackground(void) {
    DrawTexture(tx_playfield, 0, 0, WHITE);
}

/* 전경 화면을 그린다. */
static void DrawForeground(void) {
    sprintf(current_score_str, "%d", current_score);
    sprintf(highest_score_str, "%d", highest_score);

    elapsed_time = (time_t) _elapsed_time / TARGET_FPS;
    strftime(elapsed_time_str, ISTR_SZ, "%M:%S", localtime(&elapsed_time));

    /* TODO: ... */
}

/* 블록이 놓이는 공간을 그린다. */
static void DrawPlayfield(void) {
    Block *block;

    for (int py = 0; py < PF_HEIGHT; py++) {
        for (int px = 0; px < PF_WIDTH; px++) {
            block = GetBlock(px, py);
            
            UpdateBlockPosition(px, py);
            FindMatches(block);

            if (block->state == BLS_MARKED) {
                // PlayBlockAnimation(block);
                SetBlock(px, py, NULL);
            } else {
                DrawTextureRec(
                    tx_blocks,
                    (Rectangle) {
                        (float) block->type * BLOCK_SZ,
                        0.0f,
                        (float) BLOCK_SZ,
                        (float) BLOCK_SZ
                    },
                    block->pos2,
                    WHITE
                );
            }
        }
    }

    HandleMouseEvents();

    DrawTexture(tx_border, (5 * BLOCK_SZ) - 16, BLOCK_SZ - 16, WHITE);
}

/* 주어진 블록과 인접한 모든 블록을 찾는다. */
static void FindMatches(Block *block) {
    int px = toLevelX(block->pos2.x);
    int py = toLevelY(block->pos2.y);

    if (block->type > BLT_WALL && block->state < BLS_FALLING) {
        adjacent_blocks[0] = GetBlock(px - 1, py); // 바로 왼쪽 칸에 있는 블록
        adjacent_blocks[1] = GetBlock(px + 1, py); // 바로 오른쪽 칸에 있는 블록
        adjacent_blocks[2] = GetBlock(px, py - 1); // 바로 위쪽 칸에 있는 블록
        adjacent_blocks[3] = GetBlock(px, py + 1); // 바로 아래쪽 칸에 있는 블록

        for (int i = 0; i < 4; i++) {
            if (adjacent_blocks[i] != NULL
                && adjacent_blocks[i]->type == block->type
                && adjacent_blocks[i]->state == BLS_NORMAL) {
                adjacent_blocks[i]->state = BLS_MARKED;
                block->state = BLS_MARKED;
            }
        }
    }
}

/* 주어진 좌표에 있는 블록의 메모리 주소를 구한다. */
static Block *GetBlock(int px, int py) {
    if (px < 0 || py < 0 || px > PF_WIDTH || py > PF_HEIGHT)
        return NULL;
    else
        return &playfield[py][px];
}

/* 
    마우스 커서가 주어진 블록의 어느 방향에 있는지 구한다.
    
    마우스 커서가 블록의...
    - 왼쪽 방향에 있으면 -1을 반환한다.
    - 오른쪽 방향에 있으면 1을 반환한다.
    - 가운데에 있으면 0을 반환한다.
*/
static int GetRelativeCursorPosition(Block *block) {
    Vector2 cursor;

    bool left_side = false;
    bool right_side = false;

    cursor = GetMousePosition();

    left_side = CheckCollisionPointRec(
        cursor,
        (Rectangle) { 0, 0, block->pos2.x, DEFAULT_HEIGHT }
    );
    right_side = CheckCollisionPointRec(
        cursor,
        (Rectangle) { block->pos2.x + BLOCK_SZ, 0, DEFAULT_WIDTH - block->pos2.x, DEFAULT_HEIGHT }
    );

    if (left_side && !right_side)
        return -1;
    else if (!left_side && right_side)
        return 1;
    else
        return 0;
}

/* 마우스 이벤트를 처리한다. */
static void HandleMouseEvents(void) {
    Block *c_block; // 마우스 커서가 가리키는 블록

    Vector2 n_cursor; // 레벨 좌표로 변환된 마우스 좌표

    bool should_highlight = false;

    // 선택한 블록의 X좌표와 Y좌표
    int cb_x = 0, cb_y = 0;

    // 선택한 블록에 대한 마우스 커서의 상대적인 위치를 나타내는 값
    int rel_cp = 0;

    n_cursor = toLevelCoords(GetMousePosition());
    c_block = GetBlock((int) n_cursor.x, (int) n_cursor.y);

    // 마우스 왼쪽 버튼을 누르고 있는가?
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (c_block != NULL && c_block->type > BLT_WALL)
            selected_block = c_block;

        if (selected_block != NULL && selected_block->type > BLT_WALL) {
            should_highlight = true;
            rel_cp = GetRelativeCursorPosition(selected_block);

            cb_x = toLevelX(selected_block->pos2.x);
            cb_y = toLevelY(selected_block->pos2.y);

            adjacent_blocks[0] = GetBlock(cb_x - 1, cb_y); // 바로 왼쪽 칸에 있는 블록
            adjacent_blocks[1] = GetBlock(cb_x + 1, cb_y); // 바로 오른쪽 칸에 있는 블록
            adjacent_blocks[2] = GetBlock(cb_x, cb_y - 1); // 바로 위쪽 칸에 있는 블록
            adjacent_blocks[3] = GetBlock(cb_x, cb_y + 1); // 바로 아래쪽 칸에 있는 블록

            if (adjacent_blocks[0]->type == BLT_EMPTY) {
                // 마우스 커서가 선택한 블록의 왼쪽에 있는가?
                if (rel_cp < 0 && selected_block->state != BLS_FALLING) {
                    SetBlock(cb_x - 1, cb_y, selected_block);
                    SetBlock(cb_x, cb_y, NULL);

                    selected_block = GetBlock(cb_x - 1, cb_y);
                }
            }

            if (adjacent_blocks[1]->type == BLT_EMPTY) {
                // 마우스 커서가 선택한 블록의 오른쪽에 있는가?
                if (rel_cp > 0 && selected_block->state != BLS_FALLING) {
                    SetBlock(cb_x + 1, cb_y, selected_block);
                    SetBlock(cb_x, cb_y, NULL);

                    selected_block = GetBlock(cb_x + 1, cb_y);
                }
            }
        }
    } else {
        selected_block = NULL;
        should_highlight = false;
    }

    // 블록을 마우스로 선택하고 있는가?
    if (selected_block != NULL && selected_block->type > BLT_WALL) {
        DrawTextureRec(
            tx_clicked,
            (Rectangle) { (float) should_highlight * BLOCK_SZ, 0.0f, (float) BLOCK_SZ, (float) BLOCK_SZ },
            (Vector2) { (float) selected_block->pos2.x, (float) selected_block->pos2.y },
            WHITE
        );
    } else {
        // 마우스 커서가 가리키는 공간이 블록으로 채워져 있는가?
        if (c_block != NULL && c_block->type > BLT_EMPTY) {
            DrawTextureRec(
                tx_clicked,
                (Rectangle) { (float) should_highlight * BLOCK_SZ, 0.0f, (float) BLOCK_SZ, (float) BLOCK_SZ },
                (Vector2) { (float) PF_STX + (float) n_cursor.x * BLOCK_SZ, (float) PF_STY + (float) n_cursor.y * BLOCK_SZ },
                WHITE
            );
        }
    }
}

/* 주어진 블록을 마우스 커서가 가리키고 있는지 확인한다. */
static bool IsBlockSelected(Block *block) {
    Vector2 cursor = GetMousePosition();

    return CheckCollisionRecs(
        (Rectangle) { cursor.x, cursor.y, 1, 1 },
        (Rectangle) { block->pos2.x, block->pos2.y, BLOCK_SZ, BLOCK_SZ }
    );
}

/* 블록이 사라지는 애니메이션을 재생한다. */
static void PlayBlockAnimation(Block *block) {
    static float alpha;
    static float duration;

    duration = TARGET_FPS * 0.5f;
    alpha = 1.0f - (block->_frame_counter / duration);

    if (alpha < 0.0f)
        alpha = 0.0f;

    if (block->_frame_counter < duration) {
        DrawTextureRec(
            tx_blocks,
            (Rectangle) {
            (float) block->type * BLOCK_SZ,
                0.0f,
                (float) BLOCK_SZ,
                (float) BLOCK_SZ
            },
            block->pos2,
            Fade(WHITE, alpha)
        );

        block->_frame_counter++;
    }
}

/* 주어진 좌표에 블록을 놓는다. */
static void SetBlock(int px, int py, Block *block) {
    if (block != NULL)
        playfield[py][px].type = block->type;
    else
        playfield[py][px].type = BLT_EMPTY;

    playfield[py][px].state = BLS_NORMAL;
    playfield[py][px].pos2 = playfield[py][px].pos1;
}

/* 블록의 위치를 업데이트한다. */
static void UpdateBlockPosition(int px, int py) {
    Block *block1, *block2;

    block1 = GetBlock(px, py); // 선택한 블록
    block2 = GetBlock(px, py + 1); // 선택한 블록의 바로 아래 칸에 있는 블록

    /*
        1. 선택한 블록이 움직일 수 있는 블록인가?
        2. 선택한 블록의 바로 아래 칸이 비어 있는가?
        3. 선택한 블록이 바닥보다 위에 있는가?
    */
    if (block1->type > BLT_WALL && block2->type == BLT_EMPTY && py < PF_HEIGHT - 1) {
        block1->state = BLS_FALLING;
        block2->state = BLS_FALLING;

        block1->pos2.y += FALLING_SPEED;

        if (block1->pos2.y - block1->pos1.y > BLOCK_SZ) {
            SetBlock(px, py + 1, block1);
            SetBlock(px, py, NULL);
        }
    }
}

/* 게임 플레이 화면을 초기화한다. */
void InitGameplayScreen(void) {
    _elapsed_time = 0;
    result = 0;

    for (int i = 0; i < TXL_LEN; i++)
        if (tx_list[i] != NULL && !LoadResourceTx(tx_list[i], txf_list[i]))
            result = 1;

    LoadLevelFromStr(LEVEL_01, playfield);
}

/* 게임 플레이 화면을 업데이트한다. */
void UpdateGameplayScreen(void) {
    _elapsed_time++;

    DrawBackground();
    DrawForeground();
    DrawPlayfield();
}

/* 게임 플레이 화면을 종료한다. */
int FinishGameplayScreen(void) {
    return result;
}