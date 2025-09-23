/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "quad.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_assert.h>

// Matches max segments allowed by GLU
#define CACHE_SIZE 240
#define TAU (2.0f * SDL_PI_F)


static void Quad_ComputeStorageRequirementsGenericQuadrilateral(Quadric* restrict q, int numSlices, int numStacks)
{
	const unsigned numVertices = ((unsigned)numStacks + 1) * ((unsigned)numSlices + 1);
	const unsigned numIndices = 6 * (unsigned)numStacks * (unsigned)numSlices;

	SDL_assert(numVertices <= q->vertexCapacity);
	SDL_assert(numIndices <= q->indexCapacity);

	q->numVertices = numVertices;
	q->numIndices  = numIndices;
}

static unsigned Quad_GenerateIndicesGenericQuadrilateral(unsigned first,
	Quadric* restrict q, int numSlices, int numStacks, bool flip)
{
	unsigned curIdx = first;
	for (unsigned stack = 0; stack < (unsigned)numStacks; ++stack)
	{
		const unsigned stack0 = (unsigned)(numSlices + 1) * (flip ? stack : stack + 1);
		const unsigned stack1 = (unsigned)(numSlices + 1) * (flip ? stack + 1 : stack);
		for (unsigned slice = 0; slice < (unsigned)numSlices; ++slice)
		{
			q->indices[curIdx++] = stack0 + slice;
			q->indices[curIdx++] = stack1 + slice;
			q->indices[curIdx++] = stack1 + slice + 1;

			q->indices[curIdx++] = stack1 + slice + 1;
			q->indices[curIdx++] = stack0 + slice + 1;
			q->indices[curIdx++] = stack0 + slice;
		}
	}

	return curIdx;
}

void Quad_Cylinder(Quadric* q, float baseRadius, float topRadius, float height, int numSlices, int numStacks)
{
	float sinCache[CACHE_SIZE], cosCache[CACHE_SIZE];

	// Sanity check inputs
	SDL_assert(numSlices >= 2);
	SDL_assert(numStacks >= 1);
	SDL_assert(baseRadius >= 0.0f);
	SDL_assert(topRadius >= 0.0f);
	SDL_assert(height >= 0.0f);

	// Clamp slices to cache size
	numSlices = SDL_min(numSlices, CACHE_SIZE - 1);

	// Calculate required storage for mesh
	Quad_ComputeStorageRequirementsGenericQuadrilateral(q, numSlices, numStacks);

	const float deltaRadius = baseRadius - topRadius;
	const float len = SDL_sqrtf(deltaRadius * deltaRadius + height * height);
	SDL_assert(len != 0.0f);
	const float sliceStep = 1.0f / (float)numSlices;
	const float stackStep = 1.0f / (float)numStacks;

	// Pre-compute cylinder vectors
	sinCache[0] = sinCache[numSlices] = 0.0f;
	cosCache[0] = cosCache[numSlices] = 1.0f;
	for (int slice = 1; slice < numSlices; ++slice)
	{
		const float theta = TAU * sliceStep * (float)slice;
		sinCache[slice] = SDL_sinf(theta);
		cosCache[slice] = SDL_cosf(theta);
	}

	// Compute normal direction for cones
	const float invLen = 1.0f / len;
	const float normalZ = deltaRadius * invLen;
	const float sliceNormalScale = height * invLen;

	// Generate vertices
	QuadIndex curVtx = 0;
	QuadVertexNormalTexture* vertices = (QuadVertexNormalTexture*)q->vertexData;
	for (int stack = 0; stack <= numStacks; ++stack)
	{
		const float radius = baseRadius - deltaRadius * stackStep * (float)stack;
		const float z = stackStep * height * (float)stack;

		for (int slice = 0; slice <= numSlices; ++slice)
		{
			const float sinSlice = sinCache[slice];
			const float cosSlice = cosCache[slice];

			vertices[curVtx++] = (QuadVertexNormalTexture)
			{
				.x = radius * sinSlice,
				.y = radius * cosSlice,
				.z = z,
				.nx = sliceNormalScale * sinSlice,
				.ny = sliceNormalScale * cosSlice,
				.nz = normalZ,
				.u = 1.0f - sliceStep * (float)slice,
				.v = stackStep * (float)stack
			};
		}
	}

	// Generate indices
	unsigned curIdx = Quad_GenerateIndicesGenericQuadrilateral(0, q, numSlices, numStacks, false);

	SDL_assert(q->numVertices == (unsigned)curVtx);
	SDL_assert(q->numIndices == curIdx);
}

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
	Quad_ComputeStorageRequirementsGenericQuadrilateral(q, numSlices, numStacks);

	const float stackStep = 1.0f / (float)numStacks;
	const float sliceStep = 1.0f / (float)numSlices;

	// Pre-compute stack vectors
	sinStackCache[0] = sinStackCache[numSlices] = 0.0f;
	cosStackCache[0] = 1.0f;
	cosStackCache[numSlices] = -1.0f;
	for (int stack = 1; stack < numStacks; ++stack)
	{
		const float theta = SDL_PI_F * stackStep * (float)stack;
		sinStackCache[stack] = SDL_sinf(theta);
		cosStackCache[stack] = SDL_cosf(theta);
	}

	// Pre-compute slice vectors
	sinSliceCache[0] = sinSliceCache[numSlices] = 0.0f;
	cosSliceCache[0] = cosSliceCache[numSlices] = 1.0f;
	for (int slice = 1; slice < numSlices; ++slice)
	{
		const float theta = TAU * sliceStep * (float)slice;
		sinSliceCache[slice] = SDL_sinf(theta);
		cosSliceCache[slice] = SDL_cosf(theta);
	}

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
	unsigned curIdx = Quad_GenerateIndicesGenericQuadrilateral(0, q, numSlices, numStacks, true);

	SDL_assert(q->numVertices == (unsigned)curVtx);
	SDL_assert(q->numIndices == curIdx);
}
