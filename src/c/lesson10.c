/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"


typedef struct
{
	float x, y, z;
	float u, v;
} Vertex;

typedef struct
{
	Vertex vertices[3];
} Triangle;

typedef struct
{
	int numTriangles;
	Triangle* tris;
} Sector;

typedef struct
{
	float x, z;
	float yaw, pitch;
	float walkBob, walkBobTheta;
} Camera;


static SDL_GPUGraphicsPipeline* pso = NULL, * psoBlend = NULL;
static SDL_GPUBuffer* vtxBuffer = NULL;
static SDL_GPUTexture* texture = NULL;
static SDL_GPUSampler* samplers[3] = { NULL, NULL, NULL };

static bool blend = false;
static unsigned filter = 0;

static Mtx projection;
static Camera camera =
{
	.x = 0.0f, .z = 0.0f,
	.yaw = 0.0f, .pitch = 0.0f,
	.walkBob = 0.0f, .walkBobTheta = 0.0f
};
static Sector world = { .numTriangles = 0, .tris = NULL };


static void ReadLine(SDL_IOStream* restrict file, char* restrict str, int max)
{
	do
	{
		char* p = str;
		for (int n = max - 1; n > 0; --n)
		{
			int8_t c;
			if (!SDL_ReadS8(file, &c))
				break;
			(*p++) = c;
			if (c == '\n')
				break;
		}
		(*p) = '\0';
	} while (str[0] == '/' || str[0] == '\n' || str[0] == '\r');
}

static void SetupWorld(const NeHeContext* ctx)
{
	SDL_IOStream* file = NeHe_OpenResource(ctx, "Data/World.txt", "r");
	if (!file)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open \"%s\": %s", "Data/World.txt", SDL_GetError());
		return;
	}

	int numTris;
	char line[255];
	ReadLine(file, line, sizeof(line));
	SDL_sscanf(line, "NUMPOLLIES %d\n", &numTris);

	world.tris = SDL_malloc(sizeof(Triangle) * (size_t)numTris);
	world.numTriangles = numTris;
	for (int tri = 0; tri < numTris; ++tri)
	{
		for (int vtx = 0; vtx < 3; ++vtx)
		{
			ReadLine(file, line, sizeof(line));
			SDL_sscanf(line, "%f %f %f %f %f",
				&world.tris[tri].vertices[vtx].x,
				&world.tris[tri].vertices[vtx].y,
				&world.tris[tri].vertices[vtx].z,
				&world.tris[tri].vertices[vtx].u,
				&world.tris[tri].vertices[vtx].v);
		}
	}
}


static bool Lesson10_Init(NeHeContext* ctx)
{
	SetupWorld(ctx);

	SDL_GPUShader* vertexShader, * fragmentShader;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson6",
		&(const NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
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
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(Vertex, u)
		}
	};
	SDL_GPUGraphicsPipelineCreateInfo psoInfo =
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
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,  // Right-handed coordinates
			.enable_depth_clip = true  // OpenGL-like clip behaviour
		},
		.target_info =
		{
			.num_color_targets = 1
		}
	};

	// Setup blend pipeline
	const SDL_GPUTextureFormat swapchainTextureFormat = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window);
	psoInfo.target_info.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
	{
		.format = swapchainTextureFormat,
		.blend_state =
		{
			.enable_blend = true,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE
		}
	};
	psoBlend = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);
	if (!psoBlend)
	{
		SDL_ReleaseGPUShader(ctx->device, fragmentShader);
		SDL_ReleaseGPUShader(ctx->device, vertexShader);
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	// Setup regular pipeline w/ depth testing
	psoInfo.depth_stencil_state = (SDL_GPUDepthStencilState)
	{
		.compare_op = SDL_GPU_COMPAREOP_LESS,  // Pass if pixel depth value tests less than the depth buffer value
		.enable_depth_test = true,             // Enable depth testing
		.enable_depth_write = true
	};
	psoInfo.target_info.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
	{
		.format = swapchainTextureFormat
	};
	psoInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	psoInfo.target_info.has_depth_stencil_target = true;
	pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &psoInfo);
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!pso)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((texture = NeHe_LoadTexture(ctx, "Data/Mud.bmp", true, true)) == NULL)
	{
		return false;
	}

	samplers[0] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST
	});
	samplers[1] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR
	});
	samplers[2] = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
		.max_lod = FLT_MAX
	});
	if (!samplers[0] || !samplers[1] || !samplers[2])
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUSampler: %s", SDL_GetError());
		return false;
	}

	if ((vtxBuffer = NeHe_CreateBuffer(ctx, world.tris, sizeof(Triangle) * (uint32_t)world.numTriangles,
		SDL_GPU_BUFFERUSAGE_VERTEX)) == NULL)
	{
		return false;
	}

	return true;
}

static void Lesson10_Quit(NeHeContext* ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, vtxBuffer);
	for (int i = SDL_arraysize(samplers) - 1; i > 0; --i)
	{
		SDL_ReleaseGPUSampler(ctx->device, samplers[i]);
	}
	SDL_ReleaseGPUTexture(ctx->device, texture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, psoBlend);
	SDL_free(world.tris);
}

static void Lesson10_Resize(NeHeContext* ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

static void Lesson10_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
	SDL_GPUTexture* restrict swapchain, unsigned swapchainW, unsigned swapchainH)
{
	(void)swapchainW; (void)swapchainH;

	const SDL_GPUColorTargetInfo colorInfo =
	{
		.texture = swapchain,
		.clear_color = { 0.0f, 0.0f, 0.0f, 0.0f },
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE
	};

	const SDL_GPUDepthStencilTargetInfo depthInfo =
	{
		.texture = ctx->depthTexture,
		.clear_depth = 1.0f,  // Ensure depth buffer clears to furthest value
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_DONT_CARE,
		.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
		.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
		.cycle = true
	};

	// Begin pass & bind pipeline state
	SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, &depthInfo);
	SDL_BindGPUGraphicsPipeline(pass, blend ? psoBlend : pso);

	// Bind texture
	SDL_BindGPUFragmentSamplers(pass, 0, &(SDL_GPUTextureSamplerBinding)
	{
		.texture = texture,
		.sampler = samplers[filter]
	}, 1);

	// Bind world vertex buffer
	SDL_BindGPUVertexBuffers(pass, 0, &(SDL_GPUBufferBinding)
	{
		.buffer = vtxBuffer,
		.offset = 0
	}, 1);

	// Setup the camera view matrix
	Mtx modelView = Mtx_Rotation(camera.pitch, 1.0f, 0.0f, 0.0f);
	Mtx_Rotate(&modelView, 360.0f - camera.yaw, 0.0f, 1.0f, 0.0f);
	Mtx_Translate(&modelView, -camera.x, -(0.25f + camera.walkBob), -camera.z);

	// Push shader uniforms
	Mtx modelViewProj = Mtx_Multiply(&projection, &modelView);
	SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProj, sizeof(Mtx));

	// Draw world
	SDL_DrawGPUPrimitives(pass, 3u * (uint32_t)world.numTriangles, 1, 0, 0);

	SDL_EndGPURenderPass(pass);

	// Handle keyboard input
	const bool *keys = SDL_GetKeyboardState(NULL);

	const float piover180 = 0.0174532925f;

	if (keys[SDL_SCANCODE_UP])
	{
		camera.x -= SDL_sinf(camera.yaw * piover180) * 0.05f;
		camera.z -= SDL_cosf(camera.yaw * piover180) * 0.05f;
		if (camera.walkBobTheta >= 359.0f)
		{
			camera.walkBobTheta = 0.0f;
		}
		else
		{
			camera.walkBobTheta += 10.0f;
		}
		camera.walkBob = SDL_sinf(camera.walkBobTheta * piover180) / 20.0f;
	}

	if (keys[SDL_SCANCODE_DOWN])
	{
		camera.x += SDL_sinf(camera.yaw * piover180) * 0.05f;
		camera.z += SDL_cosf(camera.yaw * piover180) * 0.05f;
		if (camera.walkBobTheta <= 1.0f)
		{
			camera.walkBobTheta = 359.0f;
		}
		else
		{
			camera.walkBobTheta -= 10.0f;
		}
		camera.walkBob = SDL_sinf(camera.walkBobTheta * piover180) / 20.0f;
	}

	if (keys[SDL_SCANCODE_LEFT])     { camera.yaw += 1.0f; }
	if (keys[SDL_SCANCODE_RIGHT])    { camera.yaw -= 1.0f; }
	if (keys[SDL_SCANCODE_PAGEUP])   { camera.pitch -= 1.0f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { camera.pitch += 1.0f; }
}

static void Lesson10_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_B:
			blend = !blend;
			break;
		case SDLK_F:
			filter = (filter + 1) % SDL_arraysize(samplers);
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "Lionel Brits & NeHe's 3D World Tutorial",
	.width = 640, .height = 480,
	.createDepthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
	.init = Lesson10_Init,
	.quit = Lesson10_Quit,
	.resize = Lesson10_Resize,
	.draw = Lesson10_Draw,
	.key = Lesson10_Key
};
