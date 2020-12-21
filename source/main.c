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

#include "common.h"
#include "application.h"
#include "draw.h"

#include "sprites.h"

u32 __stacksize__ = 64 * 1024;

Buffer_t empty_buffer(void)
{
    return (Buffer_t){NULL, 0, 0};
}
Buffer_t sized_buffer(const u32 size)
{
    return (Buffer_t){malloc(size), 0, size};
}
void free_buffer(Buffer_t* buf)
{
    free(buf->data);
    clear_buffer(buf);
}
void clear_buffer(Buffer_t* buf)
{
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
}

int main(void)
{
    Application_t app;
    init_app(&app);

#ifndef CITRA_MODE
    if(R_FAILED(app.archive_result))
    {
        throw_error("Theme extdata does not exist!\nSet a default theme from the home menu.", ERROR_LEVEL_ERROR);
    }
    else
#endif
    {
    bool in_preview = false;
    int selected_menu = -1;
    int selected_menu_item = 0;

    hidSetRepeatParameters(15, 10);
    while(aptMainLoop() && !app.should_quit)
    {
        hidScanInput();

        const u32 kDown = hidKeysDown();
        const u32 kDownRep = hidKeysDownRepeat();
        const u32 kHeld = hidKeysHeld();
        const u32 kUp = hidKeysUp();

        if(kDown & KEY_START)
        {
            if(check_work_queue_empty(&app))
            {
                app.should_quit = true;
            }
            else
            {
                if(draw_choice("There is still work going on\nAre you sure you want to exit?"))
                {
                    app.should_quit = true;
                }
            }
            continue;
        }

        start_frame();

        draw_base_interface();

        end_frame();
    }
    }

    exit_app(&app);
    return 0;
}
