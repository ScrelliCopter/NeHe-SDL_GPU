#ifndef QUAD_H
#define QUAD_H

#include <stdint.h>

typedef struct
{
	float x, y, z;
	float nx, ny, nz;
} QuadVertexNormal;

typedef struct
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
} QuadVertexNormalTexture;

typedef uint32_t QuadIndex;

typedef struct
{
	void* vertexData;
	QuadIndex* indices;
	unsigned vertexCapacity, indexCapacity;
	unsigned numVertices, numIndices;
} Quadric;

void Quad_Cylinder(Quadric* q, float baseRadius, float topRadius, float height, int numSlices, int numStacks);
void Quad_Sphere(Quadric* q, float radius, int numSlices, int numStacks);

#endif//QUAD_H
