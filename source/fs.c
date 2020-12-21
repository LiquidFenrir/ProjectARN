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

#include <strings.h>

#include "fs.h"

#include <archive.h>
#include <archive_entry.h>

FS_Archive ArchiveSD = 0;

Result file_to_buf(FS_Path path, FS_Archive archive, Buffer_t* out_buf)
{
    Handle file;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&file, archive, path, FS_OPEN_READ, 0))) return -1;

    u64 size;
    FSFILE_GetSize(file, &size);
    if(size != 0)
    {
        u32 read_size = size;
        if(out_buf->data)
        {
            read_size = out_buf->capacity > read_size ? read_size : out_buf->capacity;
        }
        else
        {
            *out_buf = sized_buffer(read_size);
        }

        FSFILE_Read(file, &out_buf->size, 0, out_buf->data, read_size);
    }
    FSFILE_Close(file);

    return 0;
}

static Result read_file_from_zip(struct archive *a, const char *file_name, Buffer_t* out_buf)
{
    struct archive_entry *entry;

    Result out = 0;
    bool found = false;

    while(!found && archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        found = !strcasecmp(archive_entry_pathname(entry), file_name);
    }

    if(found)
    {
        u32 read_size = archive_entry_size(entry);
        if(out_buf->data)
        {
            read_size = out_buf->capacity > read_size ? read_size : out_buf->capacity;
        }
        else
        {
            *out_buf = sized_buffer(read_size);
        }
        out_buf->size = archive_read_data(a, out_buf->data, read_size);
    }
    else
    {
        DEBUG("Couldn't find file in zip\n");
        out = -1;
    }

    archive_read_free(a);

    return out;
}
Result zip_file_to_buf(const char *file_name, const u16 *zip_path, Buffer_t* out_buf)
{
    char path[0x106 + 1] = {0};
    utf16_to_utf8((u8*)path, zip_path, 0x106);

    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_filename(a, path, 0x4000);
    if(r != ARCHIVE_OK)
    {
        archive_read_free(a);
        return -1;
    }

    return read_file_from_zip(a, file_name, out_buf);
}
Result zip_memory_to_buf(const char *file_name, const Buffer_t* zip_buf, Buffer_t* out_buf)
{
    struct archive *a = archive_read_new();
    archive_read_support_format_zip(a);

    int r = archive_read_open_memory(a, zip_buf->data, zip_buf->size);
    if(r != ARCHIVE_OK)
    {
        archive_read_free(a);
        return -1;
    }

    return read_file_from_zip(a, file_name, out_buf);
}

Result decompress_lz_file(FS_Path file_name, FS_Archive archive, Buffer_t* out_buf)
{
    Buffer_t lz_buf = empty_buffer();
    file_to_buf(file_name, archive, &lz_buf);

    const u8* temp_buf = (u8*)lz_buf.data;
    if (temp_buf[0] != 0x11)
    {
        free_buffer(&lz_buf);
        return -1;
    }

    const u32 output_size = temp_buf[1] | ((temp_buf[2] << 8) & 0xFF00) | ((temp_buf[3] << 16) & 0xFF0000);
    *out_buf = sized_buffer(output_size);

    u32 pos = 4;
    u32 cur_written = 0;
    u8 counter = 8;
    u8 mask = 0;
    u8* output = (u8*)out_buf->data;

    while (cur_written < output_size)
    {
        if (++counter > 8) // read mask
        {
            counter = 1;
            mask = temp_buf[pos++];
            continue;
        }

        if ((mask >> (8 - counter)) & 0x01) // compressed block
        {
            int len = 0;
            int disp = 0;
            switch (temp_buf[pos] >> 4)
            {
                case 0:
                    len  = temp_buf[pos++] << 4;
                    len |= temp_buf[pos] >> 4;
                    len += 0x11;
                    break;

                case 1:
                    len  = (temp_buf[pos++] & 0x0F) << 12;
                    len |=  temp_buf[pos++] << 4;
                    len |=  temp_buf[pos] >> 4;
                    len += 0x111;
                    break;

                default:
                    len  = (temp_buf[pos] >> 4) + 1;
            }

            disp  = (temp_buf[pos++] & 0x0F) << 8;
            disp |=  temp_buf[pos++];

            for (int i = 0; i < len; ++i)
            {
                output[cur_written + i] = output[cur_written - disp - 1 + i];
            }

            cur_written += len;
        }
        else // byte literal
        {
            output[cur_written] = temp_buf[pos++];
            cur_written++;
        }
    }

    free_buffer(&lz_buf);
    return 0;
}
Result compress_lz_file_fast(FS_Path path, FS_Archive archive, const Buffer_t* in_file)
{
    // TODO
    return -1;
}

Result buf_to_file(FS_Path path, FS_Archive archive, const Buffer_t* in_file)
{
    Handle handle;
    Result res = 0;
    if (R_FAILED(res = FSUSER_OpenFile(&handle, archive, path, FS_OPEN_WRITE, 0))) return res;
    if (R_FAILED(res = FSFILE_Write(handle, NULL, 0, in_file->data, in_file->size, FS_WRITE_FLUSH))) return res;
    if (R_FAILED(res = FSFILE_Close(handle))) return res;
    return 0;
}
Result remake_file(FS_Path path, FS_Archive archive, const u32 size)
{
    Handle handle;
    if (R_SUCCEEDED(FSUSER_OpenFile(&handle, archive, path, FS_OPEN_READ, 0)))
    {
        FSFILE_Close(handle);
        FSUSER_DeleteFile(archive, path);
    }
    return FSUSER_CreateFile(archive, path, 0, size);
}