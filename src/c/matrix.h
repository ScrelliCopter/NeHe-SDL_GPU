#ifndef MATRIX_H
#define MATRIX_H

typedef struct { float x, y, z; } Vec3f;
typedef struct { float x, y, z, w; } Vec4f;
typedef union { Vec4f c[4]; float a[16]; } Mtx;

inline Mtx Mtx_Init(Vec4f d)
{
	return (Mtx)
	{
		.c[0] = { d.x, 0, 0, 0 },
		.c[1] = { 0, d.y, 0, 0 },
		.c[2] = { 0, 0, d.z, 0 },
		.c[3] = { 0, 0, 0, d.w }
	};
}

inline Mtx Mtx_InitScalar(float s)
{
	return (Mtx)
	{
		.c[0] = { s, 0, 0, 0 },
		.c[1] = { 0, s, 0, 0 },
		.c[2] = { 0, 0, s, 0 },
		.c[3] = { 0, 0, 0, s }
	};
}

inline Mtx Mtx_Translation(float x, float y, float z)
{
	return (Mtx)
	{
		.c[0] = { 1, 0, 0, 0 },
		.c[1] = { 0, 1, 0, 0 },
		.c[2] = { 0, 0, 1, 0 },
		.c[3] = { x, y, z, 1 }
	};
}

inline Mtx Mtx_Scaled(float x, float y, float z)
{
	return (Mtx)
	{
		.c[0] = { x, 0, 0, 0 },
		.c[1] = { 0, y, 0, 0 },
		.c[2] = { 0, 0, z, 0 },
		.c[3] = { 0, 0, 0, 1 }
	};
}

Mtx Mtx_Rotation(float angle, float x, float y, float z);
Mtx Mtx_Perspective(float fovy, float aspect, float near, float far);
Mtx Mtx_Orthographic(float left, float right, float bottom, float top, float near, float far);

inline Mtx Mtx_Orthographic2D(float left, float right, float bottom, float top)
{
	return Mtx_Orthographic(left, right, bottom, top, -1.0f, 1.0f);
}

Mtx Mtx_Multiply(const Mtx* restrict l, const Mtx* restrict r);
Vec4f Mtx_VectorProduct(const Mtx* l, Vec4f r);
Vec4f Mtx_VectorProject(Vec4f l, const Mtx* r);

void Mtx_Translate(Mtx* m, float x, float y, float z);
void Mtx_Scale(Mtx* m, float x, float y, float z);
void Mtx_Rotate(Mtx* m, float angle, float x, float y, float z);

#endif//MATRIX_H
