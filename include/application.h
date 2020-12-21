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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "common.h"
#include "loading.h"
#include "preview.h"
#include "draw.h"

typedef enum {
    MODE_THEMES = 0,
    MODE_SPLASHES,

    MODES_AMOUNT
} AppplicationMode;

typedef enum {
    // common
    ACTION_INSTALL,
    ACTION_DELETE,
    ACTION_APPEND,

    // themes
    ACTION_INSTALL_NO_BGM,
    ACTION_INSTALL_CLEAR_BGM,
    ACTION_INSTALL_BGM_ONLY,
    ACTION_INSTALL_SHUFFLE,

    // splashes
    ACTION_INSTALL_TOP,
    ACTION_INSTALL_BOTTOM,
} ActionType;

typedef struct Action_s Action_t;
struct Action_s {
    Action_t* next;

    ActionType type;
    Entry_List_s* act_on;
    void* extra_data;
    int entry_idx; // unused for shuffle install which scans through the list
    bool wait_on; // if true, cannot add actions until this one is complete. notably deletes
};

typedef struct {
    u32 old_time_limit;
    bool homebrew;
    bool have_audio;
    bool installed_any_theme;
    bool should_quit;

    Thread icon_loading_thread;
    Thread worker_thread;

    LightEvent stop_threads_event;
    LightEvent can_draw_icons_event;
    LightLock action_lock;
    LightLock icons_lock;

    AppplicationMode mode;
    Preview_Info_s preview;
    Entry_List_s lists[MODES_AMOUNT];

    int shuffle_count;

    Action_t *action_head, *action_tail;
    Render_Info_s render;

    Result archive_result;
    FS_Archive ArchiveHomeExt, ArchiveThemeExt;
} Application_t;

void init_app(Application_t* app);
void exit_app(Application_t* app);

bool check_work_queue_empty(Application_t* app);

#endif
