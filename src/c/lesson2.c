/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	float x, y, z;
} Vertex;

static const Vertex vertices[] =
{
	// Triangle
	{  0.0f,  1.0f, 0.0f },  // Top
	{ -1.0f, -1.0f, 0.0f },  // Bottom left
	{  1.0f, -1.0f, 0.0f },  // Bottom right
	// Quad
	{ -1.0f,  1.0f, 0.0f },  // Top left
	{  1.0f,  1.0f, 0.0f },  // Top right
	{  1.0f, -1.0f, 0.0f },  // Bottom right
	{ -1.0f, -1.0f, 0.0f }   // Bottom left
};

static const uint16_t indices[] =
{
	// Triangle
	0, 1, 2,
	// Quad
	3, 4, 5, 5, 6, 3
};


static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUBuffer* idxBuffer = NULL;

static float projection[16];


static bool Lesson2_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson2",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1 }))
	{
		return false;
	}

	const SDL_GPUVertexAttribute vertexAttribs[] =
	{
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = offsetof(Vertex, x)
		}
	};
	pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
	{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_input_state =
		{
			.vertex_buffer_descriptions = &(const SDL_GPUVertexBufferDescription)
			{
				.slot = 0,
				.pitch = sizeof(Vertex),
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX
			},
			.num_vertex_buffers = 1,
			.vertex_attributes = vertexAttribs,
			.num_vertex_attributes = SDL_arraysize(vertexAttribs)
		},
		.rasterizer_state =
		{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.target_info =
		{
			.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
			{
				.format = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window)
			},
			.num_color_targets = 1,
		}
	});
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!pso)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if (!NeHe_CreateVertexIndexBuffer(ctx, &vtxBuffer, &idxBuffer,
		vertices, sizeof(vertices),
		indices, sizeof(indices)))
	{
		return false;
	}

	return true;
}

static void Lesson2_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, idxBuffer);
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson2_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	Mtx_Perspective(projection, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson2_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd, SDL_GPUTexture* restrict swapchain)
{
	(void)ctx;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.5f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	// Begin pass & bind pipeline state
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);
	SDL_BindGPUGraphicsPipeline(pass, pso);

	// Bind vertex & index buffers
	SDL_BindGPUVertexBuffers(pass, 0, &(const SDL_GPUBufferBinding)
	{
		.buffer = vtxBuffer,
		.offset = 0
	}, 1);
	SDL_BindGPUIndexBuffer(pass, &(const SDL_GPUBufferBinding)
	{
		.buffer = idxBuffer,
		.offset = 0
	}, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	float model[16], viewproj[16];

	// Draw triangle 1.5 units to the left and 6 units into the camera
	Mtx_Translation(model, -1.5f, 0.0f, -6.0f);
	Mtx_Multiply(viewproj, projection, model);
	SDL_PushGPUVertexUniformData(cmd, 0, viewproj, sizeof(viewproj));
	SDL_DrawGPUIndexedPrimitives(pass, 3, 1, 0, 0, 0);

	// Move to the right by 3 units and draw quad
	Mtx_Translate(model, 3.0f, 0.0f, 0.0f);
	Mtx_Multiply(viewproj, projection, model);
	SDL_PushGPUVertexUniformData(cmd, 0, viewproj, sizeof(viewproj));
	SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 3, 0, 0);

	SDL_EndGPURenderPass(pass);
}


const struct AppConfig appConfig =
{
	.title = "NeHe's First Polygon Tutorial",
	.width = 640, .height = 480,
	.init = Lesson2_Init,
	.quit = Lesson2_Quit,
	.resize = Lesson2_Resize,
	.draw = Lesson2_Draw
};
