#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>

struct NeHeContext;
typedef struct NeHeSound NeHeSound;

typedef unsigned NeHeSoundFlags;
#define NEHE_SND_SYNC  (NeHeSoundFlags)0x0
#define NEHE_SND_ASYNC (NeHeSoundFlags)0x1
#define NEHE_SND_LOOP  (NeHeSoundFlags)(1 << 3)

NeHeSound* NeHe_LoadSound(struct NeHeContext* restrict ctx, const char* restrict resource);
bool NeHe_OpenSound(void);
void NeHe_CloseSound(void);
bool NeHe_PlaySound(const NeHeSound* restrict sound, NeHeSoundFlags flags);

#endif//SOUND_H
