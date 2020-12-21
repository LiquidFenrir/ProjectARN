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
*   but WITHOUT ANY WARRANTY
{

} without even the implied warranty of
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

#include "loading.h"
#include "fs.h"

void smdh_load_info(Entry_s* into, const Icon_s* from)
{
    memcpy(into->name, from->name, 0x40*sizeof(u16));
    memcpy(into->desc, from->desc, 0x80*sizeof(u16));
    memcpy(into->author, from->author, 0x40*sizeof(u16));
}
void smdh_load_icon(u16* into, const Icon_s* from)
{
    u16* dest = into + (64-48)*64;
    const u16* src = from->big_icon;
    for (int j = 0; j < 48; j += 8)
    {
        memcpy(dest, src, 48*8*sizeof(u16));
        src += 48*8;
        dest += 64*8;
    }
}

Result load_list_append_downloaded(const char* path, Entry_List_s* list)
{
    Entry_s to_add;
    memset(&to_add, 0, sizeof(to_add));
    // to_add.tp_download_id = -1;
    to_add.special = SPECIAL_NONE;

    to_add.is_zip = true;
    // these will always overwrite the entire path (except the null terminator)
    to_add.path_length = utf8_to_utf16(to_add.path.path, (u8*)path, ENTRY_PATH_SIZE);

    // Failure to load, or wrong size
    Buffer_t icon_buf = sized_buffer(0x36c0);
    if(load_entry_file("/info.smdh", &to_add, &icon_buf) != 0 || icon_buf.size != icon_buf.capacity)
    {
        free_buffer(&icon_buf);
        return -1;
    }

    Entry_s* new_entries = realloc(list->entries, (list->entries_count + 1) * sizeof(Entry_s));
    int* new_view = realloc(list->entries_view, (list->entries_viewing_count + 1) * sizeof(int));

    if(new_entries != NULL) list->entries = new_entries;
    if(new_view != NULL) list->entries_view = new_view;

    if(new_entries == NULL || new_view == NULL)
    {
        free_buffer(&icon_buf);
        return -1;
    }

    smdh_load_info(&to_add, (Icon_s*)icon_buf.data);
    free_buffer(&icon_buf);

    memcpy(&list->entries[list->entries_count], &to_add, sizeof(Entry_s));
    list->entries_view[list->entries_viewing_count] = list->entries_count;

    list->entries_count++;
    list->entries_viewing_count++;
    return 0;
}

Result load_list_entries(const Entry_List_Params_s* params, Entry_List_s* list)
{
    memset(list, 0, sizeof(*list));

    list->sort.infos[0] = (Single_Sort_s){SORT_TITLE, 1};
    list->sort.infos[1] = (Single_Sort_s){SORT_AUTHOR, 0};
    list->sort.infos[2] = (Single_Sort_s){SORT_PATH, 0};

    Handle dir_handle;
    Result res = FSUSER_OpenDirectory(&dir_handle, ArchiveSD, fsMakePath(PATH_ASCII, params->loading_path));
    if(R_FAILED(res))
    {
        return res;
    }

    Entry_Path_s path_begin;
    memset(&path_begin, 0, sizeof(path_begin));
    const int path_end = utf8_to_utf16(path_begin.path, (u8*)params->loading_path, ENTRY_PATH_SIZE);

    Buffer_t icon_buf = sized_buffer(0x36c0);
    Entry_s to_add;
    memset(&to_add, 0, sizeof(to_add));
    // to_add.tp_download_id = -1;
    to_add.special = SPECIAL_NONE;

    u32 entries_read = 0;
    while(true)
    {
        FS_DirectoryEntry dir_entry = {0};
        res = FSDIR_Read(dir_handle, &entries_read, 1, &dir_entry);
        if(R_FAILED(res) || entries_read == 0)
            break;

        if(!(dir_entry.attributes & FS_ATTRIBUTE_DIRECTORY) && strcmp(dir_entry.shortExt, "ZIP"))
            continue;

        to_add.is_zip = !strcmp(dir_entry.shortExt, "ZIP");
        // these will always overwrite the entire path (except the null terminator)
        memcpy(&to_add.path, &path_begin, sizeof(Entry_Path_s));
        memcpy(&to_add.path.path[path_end], dir_entry.name, (ENTRY_PATH_SIZE - path_end) * sizeof(u16));
        to_add.path_length = path_end;
        for(int i = 0; i < (ENTRY_PATH_SIZE - path_end); ++i)
        {
            if(dir_entry.name[i] == 0)
            {
                to_add.path_length += i;
                break;
            }
        }

        // Failure to load, or wrong size
        if(load_entry_file("/info.smdh", &to_add, &icon_buf) != 0 || icon_buf.size != icon_buf.capacity)
        {
            continue;
        }

        Entry_s* new_entries = realloc(list->entries, (list->entries_count + 1) * sizeof(Entry_s));
        int* new_view = realloc(list->entries_view, (list->entries_viewing_count + 1) * sizeof(int));

        if(new_entries != NULL) list->entries = new_entries;
        if(new_view != NULL) list->entries_view = new_view;

        if(new_entries == NULL || new_view == NULL)
        {
            break;
        }

        smdh_load_info(&to_add, (Icon_s*)icon_buf.data);

        memcpy(&list->entries[list->entries_count], &to_add, sizeof(Entry_s));
        list->entries_view[list->entries_viewing_count] = list->entries_count;

        list->entries_count++;
        list->entries_viewing_count++;
    }

    free_buffer(&icon_buf);
    FSDIR_Close(dir_handle);

    list->logo = params->logo;
    list->icons_subtexture = params->icons_subtexture;
    list->entries_per_screen = params->entries_per_screen;
    list->icons = NULL;

    return res;
}
Result load_entry_file(const char * filename, const Entry_s* entry, Buffer_t* output_buffer)
{
    if(entry->is_zip)
    {
        return zip_file_to_buf(filename+1, entry->path.path, output_buffer); //the first character will always be '/' because of the other case
    }
    else
    {
        Entry_Path_s fullpath;
        memcpy(&fullpath, &entry->path, sizeof(Entry_Path_s));
        utf8_to_utf16(&fullpath.path[entry->path_length], (u8*)filename, ENTRY_PATH_SIZE - entry->path_length);

        return file_to_buf(fsMakePath(PATH_UTF16, fullpath.path), ArchiveSD, output_buffer);
    }
}

Result save_entry_file(const char * filename, const Entry_s* entry, const Buffer_t* input_buffer)
{
    if(entry->is_zip)
    {
        return -1;
    }
    else
    {
        Entry_Path_s fullpath;
        memcpy(&fullpath, &entry->path, sizeof(Entry_Path_s));
        utf8_to_utf16(&fullpath.path[entry->path_length], (u8*)filename, ENTRY_PATH_SIZE - entry->path_length);

        return buf_to_file(fsMakePath(PATH_UTF16, fullpath.path), ArchiveSD, input_buffer);
    }
}

void icons_loading_thread_func(void * arg)
{

}

static void sort_swap(Entry_List_s* list, int id1, int id2)
{
    const int tmp = list->entries_view[id2];
    list->entries_view[id2] = list->entries_view[id1];
    list->entries_view[id1] = tmp;
}
static int sort_do_compare(const Entry_List_s* list, const int id, const Entry_s* with)
{
    const Entry_s* entry1 = &list->entries[list->entries_view[id]];
    for(int i = 0; i < SORTS_AMOUNT; ++i)
    {
        Single_Sort_s s = list->sort.infos[i];
        if(s.direction == 0) continue;

        int comparison = 0;
        switch(s.type)
        {
            case SORT_TITLE:
                comparison = memcmp(entry1->name, with->name, sizeof(entry1->name));
                break;
            case SORT_AUTHOR:
                comparison = memcmp(entry1->author, with->author, sizeof(entry1->author));
                break;
            case SORT_PATH:
                comparison = memcmp(&entry1->path, &with->path, sizeof(entry1->path));
                break;
            default:
                break;
        }

        if(comparison)
        {
            return comparison * s.direction;
        }
    }

    return 0;
}

// https://www.techiedelight.com/quicksort/
static int sort_partition(Entry_List_s* list, const int low, const int high)
{
    const Entry_s* pivot = &list->entries[list->entries_view[high]];
    // elements less than pivot will be pushed to the left of pIndex
    // elements more than pivot will be pushed to the right of pIndex
    // equal elements can go either way
    int pIndex = low;
 
    // each time we finds an element less than or equal to pivot, pIndex
    // is incremented and that element would be placed before the pivot.
    for(int i = low; i < high; i++)
    {
        if(sort_do_compare(list, i, pivot) == -1)
        {
            sort_swap(list, i, pIndex);
            pIndex++;
        }
    }
    // swap pIndex with Pivot
    sort_swap(list, pIndex, high);
 
    // return pIndex (index of pivot element)
    return pIndex;
}

static void sort_quicksort(Entry_List_s* list, int low, int high)
{
    if (low >= high)
        return;

    const int pivot = sort_partition(list, low, high);
    sort_quicksort(list, low, pivot - 1); 
    sort_quicksort(list, pivot + 1, high); 
}
void list_apply_sort(Entry_List_s* list)
{
    if(memcmp(&list->previous_sort, &list->sort, sizeof(Sort_Info_s)) == 0)
    {
        return;
    }

    sort_quicksort(list, 0, list->entries_viewing_count - 1);
    memcpy(&list->previous_sort, &list->sort, sizeof(Sort_Info_s));
}
