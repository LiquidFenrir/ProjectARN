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

#include "colors.h"

Colors_Holder_s colors_default(void)
{
    Colors_Holder_s out;
    out.background = C2D_Color32(35, 28, 32, 255); //silver-y black
    out.foreground = C2D_Color32(200, 200, 200, 255);
    out.accent = C2D_Color32(12, 58, 111, 255);
    out.warning = C2D_Color32(239, 220, 11, 255);
    out.error = C2D_Color32(229, 66, 66, 255);
    out.black = C2D_Color32(0, 0, 0, 255);
    out.white = C2D_Color32(255, 255, 255, 255);
    return out;
}
