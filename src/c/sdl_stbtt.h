#ifndef NEHE_SDL_STB_TRUETYPE
#define NEHE_SDL_STB_TRUETYPE

#include <SDL3/SDL_stdinc.h>

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
