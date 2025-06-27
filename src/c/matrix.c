/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "matrix.h"
#include <SDL3/SDL_stdinc.h>


extern inline Mtx Mtx_Init(Vec4f d);
extern inline Mtx Mtx_InitScalar(float s);
extern inline Mtx Mtx_Translation(float x, float y, float z);
extern inline Mtx Mtx_Scaled(float x, float y, float z);

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

Mtx Mtx_Rotation(float angle, float x, float y, float z)
{
	float r[9];
	MakeGLRotation(r, angle, x, y, z);
	return (Mtx)
	{
		.c[0] = { r[0], r[1], r[2], 0.0f },
		.c[1] = { r[3], r[4], r[5], 0.0f },
		.c[2] = { r[6], r[7], r[8], 0.0f },
		.c[3] = { 0.0f, 0.0f, 0.0f, 1.0f }
	};
}

Mtx Mtx_Perspective(float fovy, float aspect, float near, float far)
{
	const float h = 1.0f / SDL_tanf(fovy * (SDL_PI_F / 180.0f) * 0.5f);
	const float w = h / aspect;
	const float invClipRng = 1.0f / (near - far);
	const float zh = far * invClipRng;
	const float zl = (far * near) * invClipRng;

	return (Mtx)
	{
		.c[0] = { w, 0, 0.f,  0 },
		.c[1] = { 0, h, 0.f,  0 },
		.c[2] = { 0, 0,  zh, -1 },
		.c[3] = { 0, 0,  zl,  0 }
	};
}

Mtx Mtx_Orthographic(float left, float right, float bottom, float top, float near, float far)
{
	const float w = 2.0f / (right - left);
	const float h = 2.0f / (top - bottom);
	const float d = 1.0f / (far - near);
	const float x = -(right + left) / (right - left);
	const float y = -(top + bottom) / (top - bottom);
	const float z = -near / (far - near);

	return (Mtx)
	{
		.c[0] = { w, 0, 0, 0 },
		.c[1] = { 0, h, 0, 0 },
		.c[2] = { 0, 0, d, 0 },
		.c[3] = { x, y, z, 1 }
	};
}

extern inline Mtx Mtx_Orthographic2D(float left, float right, float bottom, float top);

Mtx Mtx_Multiply(const Mtx* restrict l, const Mtx* restrict r)
{
	Mtx m;
	float* p = m.a;
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			float a = 0.f;
			for (int j = 0; j < 4; ++j)
			{
				a += l->a[j * 4 + row] * r->a[col * 4 + j];
			}
			*p++ = a;
		}
	}
	return m;
}

Vec4f Mtx_VectorProduct(const Mtx* l, Vec4f r)
{
	return (Vec4f)
	{
		l->c[0].x * r.x + l->c[0].y * r.y + l->c[0].z * r.z + l->c[0].w * r.w,
		l->c[1].x * r.x + l->c[1].y * r.y + l->c[1].z * r.z + l->c[1].w * r.w,
		l->c[2].x * r.x + l->c[2].y * r.y + l->c[2].z * r.z + l->c[2].w * r.w,
		l->c[3].x * r.x + l->c[3].y * r.y + l->c[3].z * r.z + l->c[3].w * r.w
	};
}

Vec4f Mtx_VectorProject(const Mtx* l, Vec4f r)
{
	const float w = l->c[0].w * r.x + l->c[1].w * r.y + l->c[2].w * r.z + l->c[3].w * r.w, iw = 1.0f / w;
	return (Vec4f)
	{
		(l->c[0].x * r.x + l->c[1].x * r.y + l->c[2].x * r.z + l->c[3].x * r.w) * iw,
		(l->c[0].y * r.x + l->c[1].y * r.y + l->c[2].y * r.z + l->c[3].y * r.w) * iw,
		(l->c[0].z * r.x + l->c[1].z * r.y + l->c[2].z * r.z + l->c[3].z * r.w) * iw,
		w * iw
	};
}

void Mtx_Translate(Mtx* m, float x, float y, float z)
{
	/*
	  m = { [1 0 0 x]
	        [0 1 0 y]
	        [0 0 1 z]
	        [0 0 0 1] } * m
	*/
	m->c[3].x += x * m->c[0].x + y * m->c[1].x + z * m->c[2].x;
	m->c[3].y += x * m->c[0].y + y * m->c[1].y + z * m->c[2].y;
	m->c[3].z += x * m->c[0].z + y * m->c[1].z + z * m->c[2].z;
	m->c[3].w += x * m->c[0].w + y * m->c[1].w + z * m->c[2].w;
}

void Mtx_Scale(Mtx* m, float x, float y, float z)
{
	/*
	  m = { [x 0 0 0]
	        [0 y 0 0]
	        [0 0 z 0]
	        [0 0 0 1] } * m
	*/
	m->c[0].x *= x; m->c[0].y *= x; m->c[0].z *= x; m->c[0].w *= x;
	m->c[1].x *= y; m->c[1].y *= y; m->c[1].z *= y; m->c[1].w *= y;
	m->c[2].x *= z; m->c[2].y *= z; m->c[2].z *= z; m->c[2].w *= z;
}

void Mtx_Rotate(Mtx* m, float angle, float x, float y, float z)
{
	// Set up temporaries
	float tmp[12], r[9];
	SDL_memcpy(tmp, m->a, sizeof(float) * 12);
	MakeGLRotation(r, angle, x, y, z);

	// Partial matrix multiplication
	m->a[0]  = r[0] * tmp[0] + r[1] * tmp[4] + r[2] * tmp[8];
	m->a[1]  = r[0] * tmp[1] + r[1] * tmp[5] + r[2] * tmp[9];
	m->a[2]  = r[0] * tmp[2] + r[1] * tmp[6] + r[2] * tmp[10];
	m->a[3]  = r[0] * tmp[3] + r[1] * tmp[7] + r[2] * tmp[11];
	m->a[4]  = r[3] * tmp[0] + r[4] * tmp[4] + r[5] * tmp[8];
	m->a[5]  = r[3] * tmp[1] + r[4] * tmp[5] + r[5] * tmp[9];
	m->a[6]  = r[3] * tmp[2] + r[4] * tmp[6] + r[5] * tmp[10];
	m->a[7]  = r[3] * tmp[3] + r[4] * tmp[7] + r[5] * tmp[11];
	m->a[8]  = r[6] * tmp[0] + r[7] * tmp[4] + r[8] * tmp[8];
	m->a[9]  = r[6] * tmp[1] + r[7] * tmp[5] + r[8] * tmp[9];
	m->a[10] = r[6] * tmp[2] + r[7] * tmp[6] + r[8] * tmp[10];
	m->a[11] = r[6] * tmp[3] + r[7] * tmp[7] + r[8] * tmp[11];
}
