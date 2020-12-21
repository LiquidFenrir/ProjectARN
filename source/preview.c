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

#include "preview.h"
#include "fs.h"
#include <png.h>
#include <tremor/ivorbisfile.h>
#include <tremor/ivorbiscodec.h>

typedef struct {
    const Buffer_t* buf;
    u32 location;
} Audio_Buffer_Location_s;

static size_t audio_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    Audio_Buffer_Location_s* loc = (Audio_Buffer_Location_s*)datasource;
    u32 read_size = nmemb * size;
    size_t count = nmemb;
    const u32 max_read = loc->buf->size - loc->location;
    if(read_size > max_read)
    {
        count = (max_read / size);
        read_size = count * size;
    }
    memcpy(ptr, loc->buf->data + loc->location, read_size);
    loc->location += read_size;
    return count;
}
static int audio_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    Audio_Buffer_Location_s* loc = (Audio_Buffer_Location_s*)datasource;
    switch(whence)
    {
        case SEEK_SET:
            loc->location = offset;
            break;
        case SEEK_CUR:
            loc->location += offset;
            break;
        case SEEK_END:
            loc->location = loc->buf->size + offset;
            break;
        default:
            break;
    }
    return 0;
}
static long audio_tell_func(void *datasource)
{

    Audio_Buffer_Location_s* loc = (Audio_Buffer_Location_s*)datasource;
    return loc->location;
}

static void audio_thread_func(void* arg)
{
    Preview_Info_s* preview = (Preview_Info_s*)arg;
    // Determines volume for the 12 (?) different outputs. See https://libctru.devkitpro.org/channel_8h.html#a30eb26f1972cc3ec28370263796c0444
    float mix[12] = {0};
    mix[0] = mix[1] = 1.0f; 

    ov_callbacks callbacks;
    callbacks.read_func = audio_read_func;
    callbacks.seek_func = audio_seek_func;
    callbacks.close_func = NULL;
    callbacks.tell_func = audio_tell_func;

    const u32 BUF_TO_READ = 0x1000;
    ndspWaveBuf wave_buf[2];
    wave_buf[0].data_vaddr = linearAlloc(BUF_TO_READ); // Most vorbis packets should only be 4 KB at most (?) Possibly dangerous assumption
    wave_buf[1].data_vaddr = linearAlloc(BUF_TO_READ);

    while(true)
    {
        LightEvent_Wait(&preview->event_start);

        if(preview->data.data == NULL)
        {
            break;
        }

        Audio_Buffer_Location_s loc;
        loc.buf = &preview->data;
        loc.location = 0;

        OggVorbis_File vf;
        ov_open_callbacks(&loc, &vf, NULL, 0, callbacks);

        vorbis_info *vi = ov_info(&vf, -1);
        if(vi->channels == 2)
        {
            ndspChnWaveBufClear(0);
            ndspChnReset(0);
            ndspChnInitParams(0);
            ndspChnSetInterp(0, NDSP_INTERP_LINEAR); 
            ndspChnSetMix(0, mix); // See mix comment above
            ndspChnSetRate(0, vi->rate);// Set sample rate to what's read from the ogg file
            ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16); // 2 channels == Stereo

            wave_buf[0].status = wave_buf[1].status = NDSP_WBUF_DONE; // Used in play to stop from writing to current buffer

            int current_buffer = 0;
            while(!LightEvent_TryWait(&preview->event_stop_playback))
            {
                if(wave_buf[current_buffer].status == NDSP_WBUF_DONE)
                {
                    int bitstream;
                    long read = ov_read(&vf, (char*)wave_buf[current_buffer].data_vaddr, BUF_TO_READ, &bitstream); // read 1 vorbis packet into wave buffer
                    if(read == 0)
                    {
                        ov_pcm_seek(&vf, 0);
                        read = ov_read(&vf, (char*)wave_buf[current_buffer].data_vaddr, BUF_TO_READ, &bitstream);
                    }
                    // 4 bytes per sample (2 bytes per channel, 2 channels)
                    wave_buf[current_buffer].nsamples = read / 4;

                    ndspChnWaveBufAdd(0, &wave_buf[current_buffer]); // Add buffer to ndsp 
                    current_buffer = 1 - current_buffer; // switch to other buffer to load and prepare it while the current one is playing
                }
                else
                {
                    // time per sample in nanoseconds * quarter of number of samples per packet
                    svcSleepThread(((1000 * 1000 * 1000ULL) / vi->rate) * ((BUF_TO_READ / 4) / 4)); // loop every 4th of a "packet"
                }
            }
        }
        else
        {
            LightEvent_Wait(&preview->event_stop_playback);
        }

        ov_clear(&vf);
        LightEvent_Signal(&preview->event_done);
    }

    linearFree((void*)wave_buf[0].data_vaddr);
    linearFree((void*)wave_buf[1].data_vaddr);
}
static void preview_set_music(Buffer_t* audio, Preview_Info_s* preview)
{
    preview_stop_music_playback(preview);

    preview->data = *audio;
    clear_buffer(audio);

    if(preview->audio_thread == NULL)
    {
        preview->audio_thread = threadCreate(audio_thread_func, preview, 0x4000, 0x3F, 0, false);
    }

    LightEvent_Signal(&preview->event_start);
}

static void preview_load_png(const Buffer_t* inbuf, Preview_Info_s* preview)
{
    png_image img;
    memset(&img, 0, sizeof(img));

    png_image_begin_read_from_memory(&img, inbuf->data, inbuf->size);

    img.format = PNG_FORMAT_RGBA;
    const int width = img.width;
    Buffer_t outbuf = sized_buffer(PNG_IMAGE_SIZE(img));

    png_image_finish_read(&img, NULL, outbuf.data, PNG_IMAGE_ROW_STRIDE(img), NULL);
    png_image_free(&img);

    const int max_x = width > 512 ? 512 : width;

    preview->top_subtex.left = (max_x - 400)/512.0f;
    preview->top_subtex.right = preview->top_subtex.left + (400.0f/512.0f);
    preview->bottom_subtex.left = (max_x - 320)/512.0f;
    preview->bottom_subtex.right = preview->bottom_subtex.left + (320.0f/512.0f);

    for(int y = 0; y < 480; y++)
    {
        const int y_off = y * width;
        const u32 y_part = (y >> 3) * (512 >> 3);
        for(int x = 0; x < max_x; x++)
        {
            const u32 dst_off = (((y_part + (x >> 3)) << 6) + ((x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3)));
            ((u32*)preview->underlying.data)[dst_off] = ((u32*)outbuf.data)[y_off + x];
        }
    }

    free_buffer(&outbuf);
}

Result preview_load_from_buffer(const Buffer_t* png, Buffer_t* audio, Preview_Info_s* preview)
{
    preview_load_png(png, preview);

    if(*preview->have_audio && audio)
    {
        preview_set_music(audio, preview);
    }

    memset(&preview->loaded_path, 0, sizeof(Entry_Path_s));
    return 0;
}
Result preview_load_from_entry(const Entry_List_s* list, Preview_Info_s* preview)
{
    Entry_s* entry_ptr = &list->entries[list->entries_view[list->selected_entry]];
    if(memcmp(&preview->loaded_path, &entry_ptr->path, sizeof(Entry_Path_s)) == 0)
    {
        return 0;
    }

    Buffer_t srcbuf = empty_buffer();
    Result out = load_entry_file("/preview.png", entry_ptr, &srcbuf);
    if(R_FAILED(out)) return out;

    preview_load_png(&srcbuf, preview);
    free_buffer(&srcbuf);

    if(*preview->have_audio)
    {
        out = load_entry_file("/bgm.ogg", entry_ptr, &srcbuf);
        if(R_SUCCEEDED(out))
        {
            preview_set_music(&srcbuf, preview);
        }
    }

    memcpy(&preview->loaded_path, &entry_ptr->path, sizeof(Entry_Path_s));

    return out;
}

void preview_stop_music_playback(Preview_Info_s* preview)
{
    if(*preview->have_audio &&preview->audio_thread != NULL)
    {
        LightEvent_Signal(&preview->event_stop_playback);
        LightEvent_Wait(&preview->event_done);
        free_buffer(&preview->data);
    }
}
void preview_stop_music_thread(Preview_Info_s* preview)
{
    if(*preview->have_audio && preview->audio_thread != NULL)
    {
        if(preview->data.data != NULL)
        {
            // stop current
            LightEvent_Signal(&preview->event_stop_playback);
            LightEvent_Wait(&preview->event_done);
            // set buffer to NULL
            free_buffer(&preview->data);
        }
        // start again -> thread notices NULL -> stops
        LightEvent_Signal(&preview->event_start);
        threadJoin(preview->audio_thread, U64_MAX);
        threadFree(preview->audio_thread);
        preview->audio_thread = NULL;
    }
}
