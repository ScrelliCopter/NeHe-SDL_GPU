#ifndef NEHE_SDL_STB_TRUETYPE
#define NEHE_SDL_STB_TRUETYPE

#include <SDL3/SDL_stdinc.h>

#define stbtt_uint8       Uint8
#define stbtt_int8        Sint8
#define stbtt_uint16      Uint16
#define stbtt_int16       Sint16
#define stbtt_uint32      Uint32
#define stbtt_int32       Sint32
#define STBTT_ifloor(X)   ((int)SDL_floor(X))
#define STBTT_iceil(X)    ((int)SDL_ceil(X))
#define STBTT_sqrt(X)     SDL_sqrt(X)
#define STBTT_pow(X,Y)    SDL_pow(X,Y)
#define STBTT_fmod(X,Y)   SDL_fmod(X,Y)
#define STBTT_cos(X)      SDL_cos(X)
#define STBTT_acos(X)     SDL_acos(X)
#define STBTT_fabs(X)     SDL_fabs(X)
#define STBTT_malloc(X,U) ((void)U,SDL_malloc(X))
#define STBTT_free(X,U)   ((void)U,SDL_free(X))
#define STBTT_assert(X)   SDL_assert(X)
#define STBTT_strlen(X)   SDL_strlen(X)
#define STBTT_memcpy      SDL_memcpy
#define STBTT_memset      SDL_memset

#define STB_TRUETYPE_IMPLEMENTATION

#include "stb_truetype.h"

#endif//NEHE_SDL_STB_TRUETYPE
