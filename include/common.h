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

#ifndef COMMON_H
#define COMMON_H

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define POS() DEBUG("%s (line %d)...\n", __func__, __LINE__)

#define DEBUGPOS(...) \
        POS(); \
        DEBUG(__VA_ARGS__)

typedef struct {
    char* data;
    // byte count
    u32 size;
    u32 capacity;
} Buffer_t;

Buffer_t empty_buffer(void);
Buffer_t sized_buffer(const u32 size);
void free_buffer(Buffer_t* buf);
void clear_buffer(Buffer_t* buf);

#endif
