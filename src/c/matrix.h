#ifndef MATRIX_H
#define MATRIX_H

#include <SDL3/SDL_stdinc.h>

inline void Mtx_Identity(float m[16])
{
	SDL_memcpy(m, (float[])
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	}, sizeof(float) * 16);
}

inline void Mtx_Translation(float m[16], float x, float y, float z)
{
	SDL_memcpy(m, (float[])
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		x, y, z, 1
	}, sizeof(float) * 16);
}

inline void Mtx_Scaled(float m[16], float x, float y, float z)
{
	SDL_memcpy(m, (float[])
	{
		x, 0, 0, 0,
		0, y, 0, 0,
		0, 0, z, 0,
		0, 0, 0, 1
	}, sizeof(float) * 16);
}

void Mtx_Rotation(float m[16], float angle, float x, float y, float z);
void Mtx_Perspective(float m[16], float fovy, float aspect, float near, float far);
void Mtx_Orthographic(float m[16], float left, float right, float bottom, float top, float near, float far);

inline void Mtx_Orthographic2D(float m[16], float left, float right, float bottom, float top)
{
	Mtx_Orthographic(m, left, right, bottom, top, -1.0f, 1.0f);
}

void Mtx_Multiply(float m[16], const float l[16], const float r[16]);
void Mtx_VectorProduct(float v[4], const float l[16], const float r[4]);
void Mtx_VectorProject(float v[4], const float l[16], const float r[4]);

void Mtx_Translate(float m[16], float x, float y, float z);
void Mtx_Scale(float m[16], float x, float y, float z);
void Mtx_Rotate(float m[16], float angle, float x, float y, float z);

#endif//MATRIX_H
