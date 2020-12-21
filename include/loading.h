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

#ifndef LOADING_H
#define LOADING_H

#include "common.h"
#include <jansson.h>

typedef struct {
    u8 _padding1[4 + 2 + 2];

    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];

    u8 _padding2[0x2000 - 0x200 + 0x30 + 0x8];
    u16 small_icon[24*24];

    u16 big_icon[48*48];
} Icon_s;

enum EntrySpecial {
    SPECIAL_NONE = 0,
    SPECIAL_SHUFFLE = 1,
    SPECIAL_SHUFFLE_NO_BGM = 2,
};

#define ENTRY_PATH_SIZE 0x106
typedef struct {
    u16 path[ENTRY_PATH_SIZE + 1];
} Entry_Path_s;

typedef struct {
    u16 name[0x41];
    u16 desc[0x81];
    u16 author[0x41];

    bool is_zip;
    Entry_Path_s path;
    int path_length;

    int special;

    // json_int_t tp_download_id;
} Entry_s;

enum SortTypes {
    SORT_TITLE,
    SORT_AUTHOR,
    SORT_PATH,

    SORTS_AMOUNT
};
typedef struct {
    s8 type;
    s8 direction;
} Single_Sort_s;
typedef struct {
    Single_Sort_s infos[SORTS_AMOUNT];
} Sort_Info_s;

typedef struct {
    Entry_s* entries;
    int* entries_view;
    int entries_count;
    // used when adding/removing elements post load
    // they arent removed from the entries array but only from the view
    int entries_viewing_count;

    const char* loading_path;
    C2D_Image logo; // image from spritesheet

    Tex3DS_SubTexture icons_subtexture;
    C3D_Tex* icons;

    int entries_per_screen;

    int previous_scroll;
    int scroll;

    // in the entries_view array
    int previous_selected;
    int selected_entry;

    Sort_Info_s previous_sort, sort;
} Entry_List_s;

typedef struct {
    const char* loading_path;
    int entries_per_screen;
    Tex3DS_SubTexture icons_subtexture;
    C2D_Image logo;
} Entry_List_Params_s;

void smdh_load_info(Entry_s* into, const Icon_s* from);
void smdh_load_icon(u16* into, const Icon_s* from);

// after a download, only works for zips
Result load_list_append_downloaded(const char* path, Entry_List_s* list);
// on launch
Result load_list_entries(const Entry_List_Params_s* params, Entry_List_s* list);
Result load_entry_file(const char * filename, const Entry_s* entry, Buffer_t* output_buffer);
// only for folder entries (used by the browser to save smdh, preview)
Result save_entry_file(const char * filename, const Entry_s* entry, const Buffer_t* input_buffer);

void icons_loading_thread_func(void * arg);

void list_apply_sort(Entry_List_s* list);

#endif