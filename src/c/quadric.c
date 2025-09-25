/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "quadric.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_assert.h>

// Matches max segments allowed by GLU
#define CACHE_SIZE 240

static const float tau = 2.0f * SDL_PI_F;
static const float deg2rad = SDL_PI_F / 180.f;


static void Quad_ComputeStorageRequirementsCylindricalQuads(Quadric* restrict q, int numSlices, int numStacks)
{
	const unsigned numVertices = ((unsigned)numStacks + 1) * ((unsigned)numSlices + 1);
	const unsigned numIndices = 6 * (unsigned)numStacks * (unsigned)numSlices;

	SDL_assert(numVertices <= q->vertexCapacity);
	SDL_assert(numIndices <= q->indexCapacity);

	q->numVertices = numVertices;
	q->numIndices  = numIndices;
}

static unsigned Quad_GenerateIndicesGenericQuadrilateral(QuadIndex vtxOffset,
	QuadIndex* restrict indices, int numSlices, int numStacks, bool flip, bool contiguousSlice)
{
	if (contiguousSlice)
		--numSlices;
	const QuadIndex stackStride = (QuadIndex)numSlices + 1;
	QuadIndex stack0 = vtxOffset, stack1 = vtxOffset;
	if (flip)
		stack1 += stackStride;
	else
		stack0 += stackStride;

	unsigned curIdx = 0;
	for (int stack = 0; stack < numStacks; ++stack)
	{
		for (int slice = 0; slice < numSlices; ++slice)
		{
			indices[curIdx++] = stack0 + (QuadIndex)slice;
			indices[curIdx++] = stack1 + (QuadIndex)slice;
			indices[curIdx++] = stack1 + (QuadIndex)slice + 1;

			indices[curIdx++] = stack1 + (QuadIndex)slice + 1;
			indices[curIdx++] = stack0 + (QuadIndex)slice + 1;
			indices[curIdx++] = stack0 + (QuadIndex)slice;
		}
		if (contiguousSlice)
		{
			indices[curIdx++] = stack0 + (QuadIndex)numSlices;
			indices[curIdx++] = stack1 + (QuadIndex)numSlices;
			indices[curIdx++] = stack1;

			indices[curIdx++] = stack1;
			indices[curIdx++] = stack0;
			indices[curIdx++] = stack0 + (QuadIndex)numSlices;
		}
		stack0 += stackStride;
		stack1 += stackStride;
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
	Quad_ComputeStorageRequirementsCylindricalQuads(q, numSlices, numStacks);

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
		const float theta = tau * sliceStep * (float)slice;
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
	SDL_assert(q->numVertices == (unsigned)curVtx);

	// Generate indices
	const unsigned curIdx = Quad_GenerateIndicesGenericQuadrilateral(0, q->indices, numSlices, numStacks, true, false);
	SDL_assert(q->numIndices == curIdx);
}

void Quad_DiscPartial(Quadric* q, float innerRadius, float outerRadius, int numSlices, int numLoops,
	float startAngle, float sweepAngle)
{
	float sinCache[CACHE_SIZE], cosCache[CACHE_SIZE];

	// Sanity check inputs
	SDL_assert(numSlices >= 2);
	SDL_assert(numLoops >= 1);
	SDL_assert(outerRadius > 0.0f);
	SDL_assert(innerRadius >= 0.0f);

	// Clamp slices to cache size
	numSlices = SDL_min(numSlices, CACHE_SIZE - 1);

	// Clamp angles
	if (sweepAngle < -360.f || sweepAngle > 360.f) { sweepAngle = 360.f; }
	else if (sweepAngle < 0.f)
	{
		startAngle += sweepAngle;
		sweepAngle = -sweepAngle;
	}

	// Does our disc have a hole? Else we are drawing a filled disc
	const bool hasHole      = innerRadius > 0.0f;
	const bool isContiguous = sweepAngle == 360.0f;

	const int vertexSlices = isContiguous ? numSlices : numSlices + 1;

	// Calculate required storage for mesh
	unsigned numVertices, numIndices;
	if (hasHole)
	{
		numVertices = ((unsigned)numLoops + 1) * (unsigned)vertexSlices;
		numIndices = 6 * (unsigned)numLoops * (unsigned)numSlices;
	}
	else
	{
		numVertices = 1 + (unsigned)numLoops * (unsigned)vertexSlices;
		numIndices = 3 * (unsigned)numSlices + 6 * (unsigned)(numLoops - 1) * (unsigned)numSlices;
	}
	SDL_assert(numVertices <= q->vertexCapacity);
	SDL_assert(numIndices <= q->indexCapacity);
	q->numVertices = numVertices;
	q->numIndices  = numIndices;

	const float sliceStep = 1.0f / (float)numSlices;
	const float loopStep = 1.0f / (float)numLoops;

	const float deltaRadius = outerRadius - innerRadius;
	const float angleOffset = deg2rad * startAngle;

	// Pre-compute radial disc vectors
	for (int slice = 0; slice < vertexSlices; ++slice)
	{
		const float theta = angleOffset + deg2rad * sweepAngle * sliceStep * (float)slice;
		sinCache[slice] = SDL_sinf(theta);
		cosCache[slice] = SDL_cosf(theta);
	}
	if (isContiguous)
	{
		sinCache[numSlices] = sinCache[0];
		cosCache[numSlices] = cosCache[0];
	}

	// Generate vertices
	QuadIndex curVtx = 0;
	QuadVertexNormalTexture* vertices = (QuadVertexNormalTexture*)q->vertexData;
	if (!hasHole)
	{
		// Centre point
		vertices[curVtx++] = (QuadVertexNormalTexture)
		{
			.x = 0.0f, .y = 0.0f, .z = 0.0f,
			.nx = 0.0f, .ny = 0.0f, .nz = 1.0f,
			.u = 0.5f, .v = 0.5f
		};
		--numLoops;  // Draw one less loop as quads
	}
	for (int loop = 0; loop <= numLoops; ++loop)
	{
		const float radius = outerRadius - deltaRadius * loopStep * (float)loop;
		const float texScale = radius / outerRadius * 0.5f;
		for (int slice = 0; slice < vertexSlices; ++slice)
		{
			const float sinSlice = sinCache[slice];
			const float cosSlice = cosCache[slice];
			vertices[curVtx++] = (QuadVertexNormalTexture)
			{
				.x = radius * sinSlice,
				.y = radius * cosSlice,
				.z = 0.0f,
				.nx = 0.0f, .ny = 0.0f, .nz = 1.0f,
				.u = 0.5f + texScale * sinSlice,
				.v = 0.5f + texScale * cosSlice
			};
		}
	}
	SDL_assert(numVertices == (unsigned)curVtx);

	// Generate indices
	unsigned curIdx = 0;
	if (!hasHole)
	{
		// Draw innermost loop as triangles
		const QuadIndex loopStart = curVtx - (QuadIndex)(numVertices - 1);
		for (int slice = vertexSlices - 2; slice >= 0; --slice)
		{
			q->indices[curIdx++] = 0;
			q->indices[curIdx++] = loopStart + (QuadIndex)slice + 1;
			q->indices[curIdx++] = loopStart + (QuadIndex)slice;
		}
		if (isContiguous)
		{
			q->indices[curIdx++] = 0;
			q->indices[curIdx++] = loopStart;
			q->indices[curIdx++] = loopStart + (QuadIndex)numSlices - 1;
		}
	}
	const QuadIndex vtxBeg = hasHole ? 0 : 1;  // Offset by centre vertex when drawing filled discs
	curIdx += Quad_GenerateIndicesGenericQuadrilateral(vtxBeg, &q->indices[curIdx],
		numSlices, numLoops, true, isContiguous);
	SDL_assert(numIndices == curIdx);
}

extern inline void Quad_Disc(Quadric* q, float innerRadius, float outerRadius, int numSlices, int numLoops);

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
	Quad_ComputeStorageRequirementsCylindricalQuads(q, numSlices, numStacks);

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
		const float theta = tau * sliceStep * (float)slice;
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
	SDL_assert(q->numVertices == (unsigned)curVtx);

	// Generate indices
	const unsigned curIdx = Quad_GenerateIndicesGenericQuadrilateral(0, q->indices, numSlices, numStacks, false, false);
	SDL_assert(q->numIndices == curIdx);
}
