/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "matrix.h"


extern inline void Mtx_Identity(float m[16]);
extern inline void Mtx_Translation(float m[16], float x, float y, float z);
extern inline void Mtx_Scaled(float m[16], float x, float y, float z);

static void MakeRotation(float m[9], float c, float s, float x, float y, float z)
{
	const float rc = 1.f - c;
	const float rcx = x * rc, rcy = y * rc, rcz = z * rc;
	const float sx = x * s, sy = y * s, sz = z * s;

	m[0] = rcx * x + c;
	m[3] = rcx * y - sz;
	m[6] = rcx * z + sy;

	m[1] = rcy * x + sz;
	m[4] = rcy * y + c;
	m[7] = rcy * z - sx;

	m[2] = rcz * x - sy;
	m[5] = rcz * y + sx;
	m[8] = rcz * z + c;
}

static void MakeGLRotation(float m[9], float angle, float x, float y, float z)
{
	// Treat inputs like glRotatef
	const float theta = angle * (SDL_PI_F / 180.0f);
	const float axisMag = SDL_sqrtf(x * x + y * y + z * z);
	if (SDL_fabsf(axisMag - 1.f) > SDL_FLT_EPSILON)
	{
		x /= axisMag;
		y /= axisMag;
		z /= axisMag;
	}

	MakeRotation(m, SDL_cosf(theta), SDL_sinf(theta), x, y, z);
}

void Mtx_Rotation(float m[16], float angle, float x, float y, float z)
{
	float r[9];
	MakeGLRotation(r, angle, x, y, z);

	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0f;
	m[15] = 1.0f;

	m[0] = r[0]; m[1] = r[1]; m[2]  = r[2];
	m[4] = r[3]; m[5] = r[4]; m[6]  = r[5];
	m[8] = r[6]; m[9] = r[7]; m[10] = r[8];
}

void Mtx_Perspective(float m[16], float fovy, float aspect, float near, float far)
{
	const float h = 1.0f / SDL_tanf(fovy * (SDL_PI_F / 180.0f) * 0.5f);
	const float w = h / aspect;
	const float invClipRng = 1.0f / (far - near);
	const float zh = -(far + near) * invClipRng;
	const float zl = -(2.0f * far * near) * invClipRng;

	/*
	  [w  0  0  0]
	  [0  h  0  0]
	  [0  0 zh zl]
	  [0  0 -1  0]
	*/
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[12] = m[13] = m[15] = 0.0f;
	m[0]  =     w;
	m[5]  =     h;
	m[10] =    zh;
	m[14] =    zl;
	m[11] = -1.0f;
}

void Mtx_Multiply(float m[16], const float l[16], const float r[16])
{
	int i = 0;
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			float a = 0.f;
			for (int j = 0; j < 4; ++j)
			{
				a += l[j * 4 + row] * r[col * 4 + j];
			}
			m[i++] = a;
		}
	}
}

void Mtx_Translate(float m[16], float x, float y, float z)
{
	/*
	  m = { [1 0 0 x]
	        [0 1 0 y]
	        [0 0 1 z]
	        [0 0 0 1] } * m
	*/
	m[12] += x * m[0] + y * m[4] + z * m[8];
	m[13] += x * m[1] + y * m[5] + z * m[9];
	m[14] += x * m[2] + y * m[6] + z * m[10];
	m[15] += x * m[3] + y * m[7] + z * m[11];
}

void Mtx_Scale(float m[16], float x, float y, float z)
{
	/*
	  m = { [x 0 0 0]
	        [0 y 0 0]
	        [0 0 z 0]
	        [0 0 0 1] } * m
	*/
	m[0] *= x; m[1] *= x; m[2]  *= x; m[3]  *= x;
	m[4] *= y; m[5] *= y; m[6]  *= y; m[7]  *= y;
	m[8] *= z; m[9] *= z; m[10] *= z; m[11] *= z;
}

void Mtx_Rotate(float m[16], float angle, float x, float y, float z)
{
	// Set up temporaries
	float tmp[12], r[9];
	SDL_memcpy(tmp, m, sizeof(float) * 12);
	MakeGLRotation(r, angle, x, y, z);

	// Partial matrix multiplication
	m[0]  = r[0] * tmp[0] + r[1] * tmp[4] + r[2] * tmp[8];
	m[1]  = r[0] * tmp[1] + r[1] * tmp[5] + r[2] * tmp[9];
	m[2]  = r[0] * tmp[2] + r[1] * tmp[6] + r[2] * tmp[10];
	m[3]  = r[0] * tmp[3] + r[1] * tmp[7] + r[2] * tmp[11];
	m[4]  = r[3] * tmp[0] + r[4] * tmp[4] + r[5] * tmp[8];
	m[5]  = r[3] * tmp[1] + r[4] * tmp[5] + r[5] * tmp[9];
	m[6]  = r[3] * tmp[2] + r[4] * tmp[6] + r[5] * tmp[10];
	m[7]  = r[3] * tmp[3] + r[4] * tmp[7] + r[5] * tmp[11];
	m[8]  = r[6] * tmp[0] + r[7] * tmp[4] + r[8] * tmp[8];
	m[9]  = r[6] * tmp[1] + r[7] * tmp[5] + r[8] * tmp[9];
	m[10] = r[6] * tmp[2] + r[7] * tmp[6] + r[8] * tmp[10];
	m[11] = r[6] * tmp[3] + r[7] * tmp[7] + r[8] * tmp[11];
}
