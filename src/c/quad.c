/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "quad.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_assert.h>

// Matches max segments allowed by GLU
#define CACHE_SIZE 240


void Quad_Sphere(Quadric* q, float radius, int numSlices, int numStacks)
{
	float sinStackCache[CACHE_SIZE], cosStackCache[CACHE_SIZE];
	float sinSliceCache[CACHE_SIZE], cosSliceCache[CACHE_SIZE];

	// Sanity check inputs
	SDL_assert(numSlices >= 2);
	SDL_assert(numStacks >= 1);
	SDL_assert(radius >= 0.0f);

	// Clamp slices & stacks to cache size
	numSlices = SDL_min(numSlices, CACHE_SIZE - 1);
	numStacks = SDL_min(numSlices, CACHE_SIZE - 1);

	// Calculate required storage for mesh
	const unsigned numVertices = ((unsigned)numStacks + 1) * ((unsigned)numSlices + 1);
	const unsigned numIndices = 6 * (unsigned)numStacks * (unsigned)numSlices;
	SDL_assert(numVertices <= q->vertexCapacity);
	SDL_assert(numIndices <= q->indexCapacity);
	q->numVertices = numVertices;
	q->numIndices  = numIndices;

	// Pre-compute stack vectors
	sinStackCache[0] = sinStackCache[numSlices] = 0.0f;
	cosStackCache[0] = 1.0f;
	cosStackCache[numSlices] = -1.0f;
	for (int stack = 1; stack < numStacks; ++stack)
	{
		float theta = SDL_PI_F * (float)stack / (float)numStacks;
		sinStackCache[stack] = SDL_sinf(theta);
		cosStackCache[stack] = SDL_cosf(theta);
	}

	// Pre-compute slice vectors
	sinSliceCache[0] = sinSliceCache[numSlices] = 0.0f;
	cosSliceCache[0] = cosSliceCache[numSlices] = 1.0f;
	for (int slice = 1; slice < numSlices; ++slice)
	{
		float theta = (2.0f * SDL_PI_F) * (float)slice / (float)numSlices;
		sinSliceCache[slice] = SDL_sinf(theta);
		cosSliceCache[slice] = SDL_cosf(theta);
	}

	const float stackStep = 1.0f / (float)numStacks;
	const float sliceStep = 1.0f / (float)numSlices;

	// Generate vertices
	QuadIndex curVtx = 0;
	QuadVertexNormalTexture* vertices = (QuadVertexNormalTexture*)q->vertexData;
	for (int stack = 0; stack <= numStacks; ++stack)
	{
		const float sinStack = sinStackCache[stack];
		const float cosStack = cosStackCache[stack];
		for (int slice = 0; slice <= numSlices; ++slice)
		{
			const float sinSlice = sinSliceCache[slice];
			const float cosSlice = cosSliceCache[slice];
			vertices[curVtx++] = (QuadVertexNormalTexture)
			{
				.x = radius * sinStack * sinSlice,
				.y = radius * sinStack * cosSlice,
				.z = radius * cosStack,
				.nx = sinStack * sinSlice,
				.ny = sinStack * cosSlice,
				.nz = cosStack,
				.u = 1.0f - sliceStep * (float)slice,
				.v = 1.0f - stackStep * (float)stack
			};
		}
	}

	// Generate indices
	unsigned curIdx = 0;
	for (unsigned stack = 0; stack < (unsigned)numStacks; ++stack)
	{
		const unsigned stack0 = (unsigned)(numSlices + 1) * stack;
		const unsigned stack1 = (unsigned)(numSlices + 1) * (stack + 1);
		for (unsigned slice = 0; slice < (unsigned)numSlices; ++slice)
		{
			const QuadIndex i00 = stack1 + slice;
			const QuadIndex i10 = stack1 + slice + 1;
			const QuadIndex i01 = stack0 + slice;
			const QuadIndex i11 = stack0 + slice + 1;

			q->indices[curIdx++] = i00;
			q->indices[curIdx++] = i01;
			q->indices[curIdx++] = i11;

			q->indices[curIdx++] = i11;
			q->indices[curIdx++] = i10;
			q->indices[curIdx++] = i00;
		}
	}

	SDL_assert(numVertices == (unsigned)curVtx);
	SDL_assert(numIndices == curIdx);
}
