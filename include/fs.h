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

#ifndef FS_H
#define FS_H

#include "common.h"

extern FS_Archive ArchiveSD;

// reads, can accept a preallocated buffer
// in which case will only read up to capacity bytes (and set size accordingly)
Result file_to_buf(FS_Path path, FS_Archive archive, Buffer_t* out_buf);
Result zip_file_to_buf(const char *file_name, const u16 *zip_path, Buffer_t* out_buf);
Result zip_memory_to_buf(const char *file_name, const Buffer_t* zip_buf, Buffer_t* out_buf);

// writes
Result buf_to_file(FS_Path path, FS_Archive archive, const Buffer_t* in_file);
Result remake_file(FS_Path path, FS_Archive archive, const u32 size);

// utility
Result decompress_lz_file(FS_Path file_name, FS_Archive archive, Buffer_t* out_buf);
Result compress_lz_file_fast(FS_Path path, FS_Archive archive, const Buffer_t* in_file);

#endif
