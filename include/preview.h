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

#ifndef PREVIEW_H
#define PREVIEW_H

#include "common.h"
#include "loading.h"

typedef struct {
    Tex3DS_SubTexture top_subtex, bottom_subtex;
    C3D_Tex underlying;
    Entry_Path_s loaded_path; // check if the preview we're attempting to load is already loaded

    bool* have_audio;

    Thread audio_thread;
    LightEvent event_start, event_done, event_stop_playback;
    Buffer_t data; // takes ownership
} Preview_Info_s;

Result preview_load_from_buffer(const Buffer_t* png, Buffer_t* audio, Preview_Info_s* preview);
Result preview_load_from_entry(const Entry_List_s* list, Preview_Info_s* preview);
void preview_stop_music_playback(Preview_Info_s* preview);
void preview_stop_music_thread(Preview_Info_s* preview);

#endif