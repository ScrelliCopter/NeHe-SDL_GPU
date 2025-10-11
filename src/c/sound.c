/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "sound.h"
#include "nehe.h"


struct NeHeSound
{
	SDL_AudioSpec spec;
	Uint32 numFrames;
	Uint8 frames[];
};

NeHeSound* NeHe_LoadSound(struct NeHeContext* restrict ctx, const char* const restrict resource)
{
	SDL_assert(ctx && resource);

	SDL_AudioSpec wavSpec;
	Uint8* wavAudio;
	Uint32 wavSize;

	// Open WAVE file from resources
	char* path = NeHe_ResourcePath(ctx, resource);
	if (!path)
		return NULL;
	const bool success = SDL_LoadWAV(path, &wavSpec, &wavAudio, &wavSize);
	SDL_free(path);
	if (!success)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_LoadWAV: %s", SDL_GetError());
		return NULL;
	}

	// Calculate number of frames
	const unsigned frameSize = SDL_AUDIO_FRAMESIZE(wavSpec);
	const unsigned numFrames = wavSize / frameSize;

	// Allocate structure for sound
	const size_t audioSize = (size_t)frameSize * (size_t)numFrames;
	const size_t soundSize = sizeof(NeHeSound) + audioSize;
	NeHeSound* sound = (NeHeSound*)SDL_malloc(soundSize);
	if (!sound)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "NeHe_LoadSound: SDL_malloc returned NULL");
		SDL_free(wavAudio);
		return NULL;
	}

	// Copy header and data to sound structure
	SDL_memcpy(&sound->spec, &wavSpec, sizeof(SDL_AudioSpec));
	sound->numFrames = numFrames;
	SDL_memcpy(sound->frames, wavAudio, audioSize);

	SDL_free(wavAudio);
	return sound;
}


typedef struct
{
	const NeHeSound* restrict sound;
	Uint32 framesLeft;
} AudioLooperState;

static void SDLCALL AudioLoopCallback(void* user, SDL_AudioStream* stream, int additional, int total)
{
	(void)total;

	AudioLooperState* state = (AudioLooperState*)user;
	SDL_assert(state);
	const NeHeSound* sound = state->sound;
	if (!sound)
		return;

	const Uint32 frameSize = SDL_AUDIO_FRAMESIZE(sound->spec);
	Uint32 requested = (Uint32)additional / frameSize;  // Number of requested frames
	while (requested > 0)
	{
		// Get sound frames position & number of frames to push
		const Uint8* frames = sound->frames + (sound->numFrames - state->framesLeft) * frameSize;
		const Uint32 numFrames = SDL_min(requested, state->framesLeft);

		// Push frames
		if (!SDL_PutAudioStreamData(stream, frames, frameSize * numFrames))
			break;

		// Subtract number of consumed frames
		requested -= numFrames;
		state->framesLeft -= numFrames;

		if (state->framesLeft == 0)                // Reached end of sound
			state->framesLeft = sound->numFrames;  // Restart from the beginning
	}
}

static SDL_AudioDeviceID audioDevice = 0U;
static SDL_AudioStream* audioStream = NULL;
static AudioLooperState looperState;

bool NeHe_PlaySound(const NeHeSound* restrict sound, NeHeSoundFlags flags)
{
	// Init audio subsystem if needed
	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_InitSubSystem: %s", SDL_GetError());
			return SDL_APP_FAILURE;
		}
	}

	// Open logical device if needed
	if (!audioDevice)
	{
		if ((audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL)) == 0u)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_OpenAudioDevice: %s", SDL_GetError());
			return false;
		}
	}

	// Cut off previous stream
	if (audioStream)
	{
		SDL_FlushAudioStream(audioStream);
		SDL_DestroyAudioStream(audioStream);
		audioStream = NULL;
	}

	// If no sound was provided then we're done
	if (!sound)
		return true;

	// Get preferred device format
	SDL_AudioSpec deviceSpec;
	if (!SDL_GetAudioDeviceFormat(audioDevice, &deviceSpec, NULL))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GetAudioDeviceFormat: %s", SDL_GetError());
		return false;
	}

	// Open audio stream
	if ((audioStream = SDL_CreateAudioStream(&sound->spec, &deviceSpec)) == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_OpenAudioDeviceStream: %s", SDL_GetError());
		return false;
	}

	// Bind our new stream to the logical device
	if (!SDL_BindAudioStream(audioDevice, audioStream))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_BindAudioStream: %s", SDL_GetError());
		SDL_DestroyAudioStream(audioStream);
		audioStream = NULL;
		return false;
	}

	if (!(flags & NEHE_SND_LOOP))
	{
		// For one-shots then just shove the entire sound into the stream
		SDL_PutAudioStreamData(audioStream, sound->frames, (int)(SDL_AUDIO_FRAMESIZE(sound->spec) * sound->numFrames));
		SDL_FlushAudioStream(audioStream);
	}
	else
	{
		// For looped sounds setup get callback to constantly replenish the stream
		looperState.sound = sound;
		looperState.framesLeft = sound->numFrames;
		if (!SDL_SetAudioStreamGetCallback(audioStream, AudioLoopCallback, &looperState))
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_SetAudioStreamGetCallback: %s", SDL_GetError());
			SDL_DestroyAudioStream(audioStream);
			audioStream = NULL;
			return false;
		}
	}

	// Block until sound is done playing if we're synchronous
	if (!(flags & NEHE_SND_ASYNC))
	{
		while (SDL_GetAudioStreamAvailable(audioStream) > 0)
		{
			SDL_PumpEvents();  // Pump events to avoid the appearance of "crashing" to the user
			SDL_Delay(10);     // Short sleep to limit CPU usage
		}
		SDL_DestroyAudioStream(audioStream);
		audioStream = NULL;
	}

	return true;
}
