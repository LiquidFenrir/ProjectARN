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

#include "draw.h"
#include "colors.h"

#include "sprites.h"

Render_Info_s* render_info = NULL;

void set_screen(C3D_RenderTarget * screen)
{
    C2D_SceneBegin(screen);
}

void start_frame(void)
{
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(render_info->dynamic_text);
    C2D_TextBufClear(render_info->width_text);
    C2D_TargetClear(render_info->top_screen, render_info->colors.background);
    C2D_TargetClear(render_info->bottom_screen, render_info->colors.background);
}

void end_frame(void)
{
    C3D_FrameEnd(0);
}

static void draw_message_internal(const char* message, const u32 color)
{
    draw_base_interface();

    set_screen(render_info->top_screen);

    C2D_Text text;
    C2D_TextParse(&text, render_info->dynamic_text, message);
    float height = 0.0f;
    C2D_TextGetDimensions(&text, 0.8f, 0.8f, NULL, &height);

    C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, 0.0f, (240.0f - height)/2.0f, 0.0f, 0.8f, 0.8f, color);
}
bool draw_choice(const char* message)
{
    static C2D_Text yes_text, no_text;
    static bool have_text = false;
    static float yes_h, no_h;
    if(!have_text)
    {
        have_text = true;
        C2D_TextParse(&yes_text, render_info->static_text, "\uE000 Yes");
        C2D_TextParse(&no_text, render_info->static_text, "\uE001 No");
        C2D_TextGetDimensions(&yes_text, 0.8f, 0.8f, NULL, &yes_h);
        C2D_TextGetDimensions(&no_text, 0.8f, 0.8f, NULL, &no_h);
    }

    while(aptMainLoop())
    {
        start_frame();

        draw_message_internal(message, render_info->colors.foreground);

        set_screen(render_info->bottom_screen);
        
        C2D_DrawText(&yes_text, C2D_WithColor | C2D_AlignCenter, 0.0f, (120.0f - yes_h)/2.0f, 0.0f, 0.8f, 0.8f, render_info->colors.foreground);
        C2D_DrawText(&no_text, C2D_WithColor | C2D_AlignCenter, 0.0f, 120.0f + (120.0f - no_h)/2.0f, 0.0f, 0.8f, 0.8f, render_info->colors.foreground);

        end_frame();

        hidScanInput();
        u32 kDown = hidKeysDown();
        if(kDown & KEY_A) return true;
        if(kDown & KEY_B) return false;
    }
    return false;
}
void draw_loading(const char* message)
{
    start_frame();
    
    draw_message_internal(message, render_info->colors.foreground);
    
    end_frame();
}

void throw_error(const char* message, ErrorLevel level)
{
    static C2D_Text text;
    static bool have_text = false;
    static float text_h;
    if(!have_text)
    {
        have_text = true;
        C2D_TextParse(&text, render_info->static_text, "Press any key to acknowledge.");
        C2D_TextGetDimensions(&text, 0.5f, 0.5f, NULL, &text_h);
    }

    u32 color;
    switch(level)
    {
        case ERROR_LEVEL_ERROR:
            color = render_info->colors.error;
            break;
        case ERROR_LEVEL_WARNING:
            color = render_info->colors.warning;
            break;
        default:
            color = render_info->colors.foreground;
            break;
    }

    while(aptMainLoop())
    {
        start_frame();

        draw_message_internal(message, render_info->colors.foreground);

        set_screen(render_info->bottom_screen);
        
        C2D_DrawText(&text, C2D_WithColor | C2D_AlignCenter, 0.0f, (240.0f - text_h)/2.0f, 0.0f, 0.5f, 0.5f, color);

        end_frame();

        hidScanInput();
        if (hidKeysDown()) break;
    }
}

void draw_loading_bar(const char* message, const u32 current, const u32 max)
{
    start_frame();

    draw_message_internal(message, render_info->colors.foreground);

    set_screen(render_info->bottom_screen);

    const float percent = (100.0f * current) / max;
    const u32 width = 2 * (u32)percent;

    C2D_DrawRectSolid(60-3, 110-3, 0.25f, 200+6, 20+6, render_info->colors.foreground);
    C2D_DrawRectSolid(60-1, 110-1, 0.5f, 200+2, 20+2, render_info->colors.background);
    C2D_DrawRectSolid(60, 110, 0.75f, width, 20, render_info->colors.accent);

    end_frame();
}

void draw_base_interface(void)
{
    set_screen(render_info->bottom_screen);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 24, render_info->colors.accent);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 24, render_info->colors.accent);

    set_screen(render_info->top_screen);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 24, render_info->colors.accent);
}