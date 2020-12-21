/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-Present Contributors in CONTRIBUTORS.md
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#ifndef DRAW_H
#define DRAW_H

#include "common.h"
#include "loading.h"
#include "colors.h"

typedef struct {
    C3D_RenderTarget *top_screen, *bottom_screen;
    C2D_TextBuf static_text, dynamic_text, width_text;
    C2D_SpriteSheet sprites;
    Colors_Holder_s colors;
} Render_Info_s;

extern Render_Info_s* render_info;

typedef enum {
    ERROR_LEVEL_ERROR,
    ERROR_LEVEL_WARNING,
} ErrorLevel;

void start_frame(void);
void end_frame(void);
void set_screen(C3D_RenderTarget * screen);

void throw_error(const char* message, ErrorLevel level);

void draw_preview(C2D_Image top_preview, C2D_Image bottom_preview);

bool draw_choice(const char* message);

void draw_loading(const char* message);
void draw_loading_bar(const char* message, const u32 current, const u32 max);

void draw_base_interface(void);
void draw_interface(Entry_List_s* list);

#endif
