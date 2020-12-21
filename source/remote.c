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

#include "remote.h"
#include "draw.h"
#include <ctype.h>

static Result http_follow_redirects(const char* url, httpcContext* context, u32* status_code)
{
    Result ret = 0;
    char *new_url = NULL;
    do {
        ret = httpcOpenContext(context, HTTPC_METHOD_GET, url, 1);
        if (ret != 0)
        {
            httpcCloseContext(context);
            ret = -1;
            break;
        }
        ret = httpcSetSSLOpt(context, SSLCOPT_DisableVerify); // should let us do https
        ret = httpcSetKeepAlive(context, HTTPC_KEEPALIVE_ENABLED);
        ret = httpcAddRequestHeaderField(context, "User-Agent", USER_AGENT);
        ret = httpcAddRequestHeaderField(context, "Connection", "Keep-Alive");

        ret = httpcBeginRequest(context);
        if (ret != 0)
        {
            httpcCloseContext(context);
            ret = -1;
            break;
        }

        ret = httpcGetResponseStatusCode(context, status_code);
        if(ret!=0){
            httpcCloseContext(context);
            ret = -1;
            break;
        }

        const u32 status = *status_code;
        if ((status >= 301 && status <= 303) || (status >= 307 && status <= 308))
        {
            if (new_url == NULL) new_url = malloc(0x1000);
            ret = httpcGetResponseHeader(context, "Location", new_url, 0x1000);
            url = new_url;
            httpcCloseContext(context);
        }
        else
        {
            break;
        }
    } while(true);
    
    if(new_url)
    {
        free(new_url);
    }

    return ret;
}
static Result http_do_request(httpcContext* context, char ** out_filename, Buffer_t* out_buf, const char* loading_message)
{
    Result ret = 0;
    u32 content_size = 0;

    ret = httpcGetDownloadSizeState(context, NULL, &content_size);
    if (ret != 0)
    {
        return -1;
    }

    if(out_buf->capacity == 0)
    {
        *out_buf = sized_buffer(0x1000);
        if (out_buf->data == NULL)
        {
            return -1;
        }
    }

    if(out_filename)
    {
        char *content_disposition = calloc(1024, sizeof(char));
        ret = httpcGetResponseHeader(context, "Content-Disposition", content_disposition, 1024);
        if (ret != 0)
        {
            free(content_disposition);
            return -1;
        }

        char * tok = strstr(content_disposition, "filename");
        if(!(tok))
        {
            free(content_disposition);
            throw_error("Target is not valid!", ERROR_LEVEL_WARNING);
            return -1;
        }

        tok += sizeof("filename") - 1;
        if(ispunct((int)*tok))
        {
            tok++;
        }
        char* last_char = tok + strlen(tok) - 1;
        if(ispunct((int)*last_char))
        {
            *last_char = '\0';
        }

        const char illegal_characters[] = "\"?;:<>/\\+";
        for (size_t i = 0; i < strlen(tok); i++)
        {
            for (size_t n = 0; n < strlen(illegal_characters); n++)
            {
                if ((tok)[i] == illegal_characters[n])
                {
                    (tok)[i] = '-';
                }
            }
        }

        *out_filename = calloc(1024, sizeof(char));
        strcpy(*out_filename, tok);
        free(content_disposition);
    }

    do {
        u32 read_size = 0;
        ret = httpcDownloadData(context, (u8*)out_buf->data + out_buf->size, out_buf->capacity - out_buf->size, &read_size);
        out_buf->size += read_size;

        if(content_size && loading_message)
            draw_loading_bar(loading_message, out_buf->size, content_size);

        if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
        {
            if(out_buf->size == out_buf->capacity)
            {
                out_buf->capacity += 0x1000;
                char* new_buf = realloc(out_buf->data, out_buf->capacity);
                if (new_buf == NULL)
                {
                    free_buffer(out_buf);
                    return -1;
                }
                out_buf->data = new_buf;
            }
        }
    } while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

    return 0;
}
Result http_get(const char *url, char ** out_filename, Buffer_t* out_buf, const char* loading_message)
{
    Result ret;
    httpcContext context;
    u32 status_code = 0;

    if(loading_message) draw_loading(loading_message);

    ret = http_follow_redirects(url, &context, &status_code);
    if (ret != 0 || status_code != 200)
    {
        httpcCloseContext(&context);
        return -1;
    }

    ret = http_do_request(&context, out_filename, out_buf, loading_message);
    httpcCloseContext(&context);

    return ret;
}
