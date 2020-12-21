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

#include "application.h"
#include "fs.h"
#include "sprites.h"

static void worker_thread_func(void* arg)
{
    Application_t* app = (Application_t*)arg;
    while(!LightEvent_TryWait(&app->stop_threads_event))
    {
        LightLock_Lock(&app->action_lock);
        Action_t* action = app->action_head;
        LightLock_Unlock(&app->action_lock);

        if(action != NULL)
        {
            // do work

            // then advance queue
            LightLock_Lock(&app->action_lock);
            // if we were on the last action in the list, then there's nothing left
            // -> both head and tail are now NULL
            // otherwise, tail is valid and head is the next action
            if(action->next == NULL)
            {
                app->action_tail = NULL;
            }
            app->action_head = action->next;
            free(action);
            LightLock_Unlock(&app->action_lock);
        }
        else
        {
            svcSleepThread(100 * 1000 * 1000ULL);
        }
    }
}

static void init_services(Application_t* app)
{
    consoleDebugInit(debugDevice_SVC);
    cfguInit();
    acInit();
    httpcInit(0);
    app->have_audio = R_SUCCEEDED(ndspInit());

    if (R_SUCCEEDED(APT_GetAppCpuTimeLimit(&app->old_time_limit)))
    {
        APT_SetAppCpuTimeLimit(30);
    }
    aptSetHomeAllowed(false);

    if (envIsHomebrew())
    {
        s64 out;
        svcGetSystemInfo(&out, 0x10000, 0);
        app->homebrew = !out;
    }
}
static void init_variables(Application_t* app)
{
    app->installed_any_theme = false;
    app->should_quit = false;

    LightEvent_Init(&app->stop_threads_event, RESET_STICKY);
    LightEvent_Init(&app->can_draw_icons_event, RESET_ONESHOT);
    LightLock_Init(&app->action_lock);
    LightLock_Init(&app->icons_lock);

    app->icon_loading_thread = NULL;

    app->mode = MODE_THEMES;

    app->action_head = NULL;
    app->action_tail = NULL;

    app->worker_thread = threadCreate(worker_thread_func, app, 0x1000, 0x30, 1, false);
}
static void init_graphics(Application_t* app)
{
    romfsInit();

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    render_info = &app->render;
    render_info->colors = colors_default();

    render_info->top_screen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    render_info->bottom_screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    render_info->static_text = C2D_TextBufNew(1024);
    render_info->dynamic_text = C2D_TextBufNew(1024);
    render_info->width_text = C2D_TextBufNew(512);
    render_info->sprites = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
}
static void init_preview(Application_t* app)
{
    app->preview.top_subtex.width = 400;
    app->preview.top_subtex.height = 240;
    app->preview.top_subtex.top = 1.0f;
    app->preview.top_subtex.bottom = app->preview.top_subtex.top - (240.0f/512.0f);

    app->preview.bottom_subtex.width = 320;
    app->preview.bottom_subtex.height = 240;
    app->preview.bottom_subtex.top = app->preview.top_subtex.bottom;
    app->preview.bottom_subtex.bottom = app->preview.bottom_subtex.top - (240.0f/512.0f);

    app->preview.audio_thread = NULL;
    app->preview.have_audio = &app->have_audio;
    app->preview.data = empty_buffer();
    LightEvent_Init(&app->preview.event_start, RESET_ONESHOT);
    LightEvent_Init(&app->preview.event_stop_playback, RESET_ONESHOT);
    LightEvent_Init(&app->preview.event_done, RESET_ONESHOT);

    memset(&app->preview.loaded_path, 0, sizeof(app->preview.loaded_path));

    C3D_TexInit(&app->preview.underlying, 512, 512, GPU_RGBA8);
}
static void init_filesystem(Application_t* app)
{
    u8 regionCode;
    u32 archive1;
    u32 archive2;

    FS_Path home;
    FS_Path theme;

    CFGU_SecureInfoGetRegion(&regionCode);
    switch(regionCode)
    {
        case 0:
            archive1 = 0x000002cc;
            archive2 = 0x00000082;
            break;
        case 1:
            archive1 = 0x000002cd;
            archive2 = 0x0000008f;
            break;
        case 2:
            archive1 = 0x000002ce;
            archive2 = 0x00000098;
            break;
        default:
            archive1 = 0x00;
            archive2 = 0x00;
    }

    if(R_FAILED(app->archive_result = FSUSER_OpenArchive(&ArchiveSD, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return;

    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/Themes"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/Splashes"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds"), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE), FS_ATTRIBUTE_DIRECTORY);
    FSUSER_CreateDirectory(ArchiveSD, fsMakePath(PATH_ASCII, "/3ds/"  APP_TITLE  "/cache"), FS_ATTRIBUTE_DIRECTORY);

    u32 homeMenuPath[3] = {MEDIATYPE_SD, archive2, 0};
    home.type = PATH_BINARY;
    home.size = 0xC;
    home.data = homeMenuPath;
    if(R_FAILED(app->archive_result = FSUSER_OpenArchive(&app->ArchiveHomeExt, ARCHIVE_EXTDATA, home))) return;

    u32 themePath[3] = {MEDIATYPE_SD, archive1, 0};
    theme.type = PATH_BINARY;
    theme.size = 0xC;
    theme.data = themePath;
    if(R_FAILED(app->archive_result = FSUSER_OpenArchive(&app->ArchiveThemeExt, ARCHIVE_EXTDATA, theme))) return;

    Handle test_handle;
    if(R_FAILED(app->archive_result = FSUSER_OpenFile(&test_handle, app->ArchiveThemeExt, fsMakePath(PATH_ASCII, "/ThemeManage.bin"), FS_OPEN_READ, 0))) return;
    FSFILE_Close(test_handle);
}

void init_app(Application_t* app)
{
    memset(app, 0, sizeof(*app));

    init_services(app);
    init_variables(app);
    init_graphics(app);
    init_preview(app);
    init_filesystem(app);

    const char* loading_messages[MODES_AMOUNT] = {
        "Loading themes...",
        "Loading splashes...",
    };

    const Tex3DS_SubTexture smdh_subtexture = {48, 48, 0.0f, 48/64.0f, 48/64.0f, 0.0f };
    Entry_List_Params_s params[MODES_AMOUNT] = {
        {
            "/Themes/",
            4,
            smdh_subtexture,
            C2D_SpriteSheetGetImage(render_info->sprites, sprites_themes_idx),
        },
        {
            "/Splashes/",
            4,
            smdh_subtexture,
            C2D_SpriteSheetGetImage(render_info->sprites, sprites_splashes_idx),
        },
    };

    Buffer_t smdh_buf = sized_buffer(0x36c0);
    const char* const icon_loading_message = "Loading icons...";
    for(int i = 0; i < MODES_AMOUNT; i++)
    {
        draw_loading(loading_messages[i]);

        Entry_List_s* current_list = &app->lists[i];

        Result res = load_list_entries(&params[i], current_list);
        if(R_SUCCEEDED(res))
        {
            list_apply_sort(current_list);
            draw_loading(icon_loading_message);

            const int max_icons = current_list->entries_per_screen * 3;
            current_list->icons = malloc(max_icons * sizeof(C3D_Tex));
            for(int icon_off = 0; icon_off < max_icons; icon_off++)
            {
                C3D_TexInit(&current_list->icons[icon_off], 64, 64, GPU_RGB565);
            }

            if(current_list->entries_viewing_count <= max_icons)
            {
                for(int i = 0; i < current_list->entries_viewing_count; i++)
                {
                    draw_loading_bar(icon_loading_message, i, current_list->entries_viewing_count);
                    const Entry_s* entry_ptr = &current_list->entries[current_list->entries_view[i]];
                    load_entry_file("/info.smdh", entry_ptr, &smdh_buf);
                    smdh_load_icon((u16*)current_list->icons[i].data, (Icon_s*)smdh_buf.data);
                }
            }
            else
            {
                const int starti = -current_list->entries_per_screen;
                const int endi = current_list->entries_per_screen * 2;
                for(int i = starti; i < endi; i++)
                {
                    draw_loading_bar(icon_loading_message, i - starti, endi - starti);
                    int offset = i;
                    if(offset < 0)
                    {
                        offset += current_list->entries_viewing_count;
                    }

                    const Entry_s* entry_ptr = &current_list->entries[current_list->entries_view[offset]];
                    load_entry_file("/info.smdh", entry_ptr, &smdh_buf);
                    smdh_load_icon((u16*)current_list->icons[i-starti].data, (Icon_s*)smdh_buf.data);
                }
            }
            
        }
    }
    free_buffer(&smdh_buf);
    
    app->icon_loading_thread = threadCreate(icons_loading_thread_func, app, 0x1000, 0x30, 1, false);
}

void exit_app(Application_t* app)
{
    LightEvent_Signal(&app->stop_threads_event);

    preview_stop_music_thread(&app->preview);

    threadJoin(app->worker_thread, U64_MAX);
    threadFree(app->worker_thread);
    threadJoin(app->icon_loading_thread, U64_MAX);
    threadFree(app->icon_loading_thread);

    while(app->action_head != NULL)
    {
        Action_t* head = app->action_head;
        app->action_head = head->next;
        free(head->extra_data);
        free(head);
    }

    for(int i = 0; i < MODES_AMOUNT; i++)
    {
        Entry_List_s* current_list = &app->lists[i];
        for(int icon_idx = 0; icon_idx < (current_list->entries_per_screen * 3); ++icon_idx)
        {
            C3D_TexDelete(&current_list->icons[icon_idx]);
        }
        free(current_list->entries);
        free(current_list->entries_view);
        free(current_list->icons);
    }

    if(app->ArchiveThemeExt) FSUSER_CloseArchive(app->ArchiveThemeExt);
    if(app->ArchiveHomeExt) FSUSER_CloseArchive(app->ArchiveHomeExt);
    if(ArchiveSD) FSUSER_CloseArchive(ArchiveSD);

    C2D_SpriteSheetFree(render_info->sprites);

    C2D_TextBufDelete(render_info->width_text);
    C2D_TextBufDelete(render_info->dynamic_text);
    C2D_TextBufDelete(render_info->static_text);

    C3D_TexDelete(&app->preview.underlying);

    C2D_Fini();
    C3D_Fini();
    gfxExit();

    romfsExit();

    if (app->have_audio)
    {
        ndspExit();
    }
    httpcExit();
    acExit();
    cfguExit();
    if (app->old_time_limit != UINT32_MAX)
    {
        APT_SetAppCpuTimeLimit(app->old_time_limit);
    }

    // exited with start and installed a theme
    // -> restart
    // otherwise: power button -> restart ; no theme installed -> exit normally
    if(app->should_quit && app->installed_any_theme)
    {
        if(app->homebrew)
        {
            APT_HardwareResetAsync();
        }
        else
        {
            srvPublishToSubscriber(0x202, 0);
        }
    }
}

bool check_work_queue_empty(Application_t* app)
{
    LightLock_Lock(&app->action_lock);
    const bool empty = app->action_head == NULL;
    LightLock_Unlock(&app->action_lock);
    return empty;
}