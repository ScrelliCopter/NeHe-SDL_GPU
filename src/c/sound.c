/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "sound.h"
#include "nehe.h"


struct NeHeSound
{
	SDL_AudioSpec spec;
	int bytes;
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

	// Allocate structure for sound
	const size_t audioSize = (size_t)wavSize;
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
	sound->bytes = (int)wavSize;
	SDL_memcpy(sound->frames, wavAudio, audioSize);

	SDL_free(wavAudio);
	return sound;
}


typedef struct
{
	const NeHeSound* restrict sound;
	int bytesLeft;
} AudioLooperState;

static void SDLCALL AudioLoopCallback(void* user, SDL_AudioStream* stream, int additional, int total)
{
	(void)total;

	AudioLooperState* state = (AudioLooperState*)user;
	SDL_assert(state);
	const NeHeSound* sound = state->sound;
	if (!sound)
		return;

	while (additional > 0)
	{
		// Get sound frames position & number of frames to push
		const Uint8* frames = sound->frames + (sound->bytes - state->bytesLeft);
		const int framesLen = SDL_min(additional, state->bytesLeft);

		// Push frames
		if (!SDL_PutAudioStreamData(stream, frames, framesLen))
			break;

		// Subtract number of consumed frames
		additional -= framesLen;
		state->bytesLeft -= framesLen;

		if (state->bytesLeft == 0)            // Reached end of sound
			state->bytesLeft = sound->bytes;  // Restart from the beginning
	}
}

static SDL_AudioDeviceID audioDevice = 0U;
static SDL_AudioStream* audioStream = NULL;
static AudioLooperState looperState;

bool NeHe_OpenSound(void)
{
	SDL_assert(audioDevice == 0u);

	// Init audio subsystem if needed
	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_InitSubSystem: %s", SDL_GetError());
			return false;
		}
	}

	// Open logical device
	if (!audioDevice)
	{
		if ((audioDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL)) == 0u)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_OpenAudioDevice: %s", SDL_GetError());
			return false;
		}
	}

	return true;
}

static void StopSound(void)
{
	if (audioStream == NULL)
		return;

	SDL_FlushAudioStream(audioStream);
	SDL_DestroyAudioStream(audioStream);
	audioStream = NULL;
}

void NeHe_CloseSound(void)
{
	StopSound();                        // Stop & dispose of currently playing stream if any
	SDL_CloseAudioDevice(audioDevice);  // Close the logical audio device
	audioDevice = 0u;
}

bool NeHe_PlaySound(const NeHeSound* restrict sound, NeHeSoundFlags flags)
{
	// Open device if needed
	if (audioDevice == 0u && !NeHe_OpenSound())
		return false;

	StopSound();        // Cut off previous stream
	if (sound == NULL)  // If no sound was provided then we're done
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
		SDL_PutAudioStreamData(audioStream, sound->frames, sound->bytes);
		SDL_FlushAudioStream(audioStream);
	}
	else
	{
		// For looped sounds setup get callback to constantly replenish the stream
		looperState.sound = sound;
		looperState.bytesLeft = sound->bytes;
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
