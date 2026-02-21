/* Reimplementation of the engine in C */

#include "amberstar.h"
#include "tester.h"
#include <SDL3/SDL_endian.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_storage.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stdio.h>

const char *data_root = "data";

char *data_path(const char *data_root, const char *filename)
{
    static char buf[512];
    (void)SDL_snprintf(buf, sizeof(buf), "%s/%s", data_root, filename);
    return buf;
}

const char *amb_error_str(int code)
{
    switch (code) {
        case AMB_OK:                return "OK";
        case AMB_ERR_OPEN_OR_SIZE:  return "open or size fail";
        case AMB_ERR_BAD_MAGIC:     return "bad AMB magic";
        case AMB_ERR_NOT_AMBR:      return "expected AMBR (AUTOMAP)";
        case AMB_ERR_OPEN_AMPC:    return "open AMPC file fail";
        case AMB_ERR_NOT_AMPC:     return "expected AMPC";
        default:                   return "unknown error";
    }
}

t_file_res Load_file(const char *data_root, const char *filename)
{
    t_file_res out = { NULL, 0 };
    char *path = data_path(data_root, filename);
    SDL_IOStream *io = SDL_IOFromFile(path, "rb");
    if (!io)
        return out;
    Sint64 size = SDL_GetIOSize(io);
    if (size <= 0 || size > (Sint64)(256 * 1024 * 1024)) {  /* cap 256 MiB */
        SDL_CloseIO(io);
        return out;
    }
    void *ptr = SDL_malloc((size_t)size);
    if (!ptr) {
        SDL_CloseIO(io);
        return out;
    }
    if (SDL_ReadIO(io, ptr, (size_t)size) != (size_t)size) {
        SDL_free(ptr);
        SDL_CloseIO(io);
        return out;
    }
    SDL_CloseIO(io);
    out.ptr = ptr;
    out.len = (uint32_t)size;
    return out;
}

t_subfile_res Load_subfile(const char *data_root, const char *filename, int subfile_idx)
{
    char *path = data_path(data_root, filename);
    (void)subfile_idx;
    (void)path;
    SDL_IOFromFile(path, "rb");
    return (t_subfile_res){ NULL, 0 };  /* TODO: open, read 6-byte header, lengths[], seek, read block */
}

int main(void)
{
    if(!SDL_Init(SDL_INIT_VIDEO))
        return -1;;
    SDL_Window* window = SDL_CreateWindow("amberstar",300, 200, 0);
    SDL_ShowWindow(window);
    SDL_Delay(1000);
    SDL_Quit();
    int res = test_open_amb();
    printf("%d %s\n", res, amb_error_str(res));
    hexdump_amb_header("PICS80.AMB", 64);
    test_open_amb();  /* 6-byte header + first 14 subfile lengths = 62 bytes */
    return res;
}