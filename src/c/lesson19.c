/*
 * SPDX-FileCopyrightText: (C) 2025 a dinosaur
 * SPDX-License-Identifier: Zlib
 */

#include "nehe.h"

#define MAX_PARTICLES 1000


typedef struct { float x, y; } Vec2f;
typedef struct { float x, y, z; } Vec3f;
typedef struct { float r, g, b; } Color;

typedef struct
{
	Vec3f position, velocity;
	Color color;
	float life, decay;
} Particle;

static struct ParticleSystem
{
	Particle particles[MAX_PARTICLES];
	Vec2f gravity, constant;
	float slowDown;
	unsigned cycleDelay, colorIndex;
	bool autoCycle;
} system;

// Rainbow table
static const Color particleColors[12] =
{
	{ .r = 1.0f,  .g = 0.5f,  .b = 0.5f  },
	{ .r = 1.0f,  .g = 0.75f, .b = 0.5f  },
	{ .r = 1.0f,  .g = 1.0f,  .b = 0.5f  },
	{ .r = 0.75f, .g = 1.0f,  .b = 0.5f  },
	{ .r = 0.5f,  .g = 1.0f,  .b = 0.5f  },
	{ .r = 0.5f,  .g = 1.0f,  .b = 0.75f },
	{ .r = 0.5f,  .g = 1.0f,  .b = 1.0f  },
	{ .r = 0.5f,  .g = 0.75f, .b = 1.0f  },
	{ .r = 0.5f,  .g = 0.5f,  .b = 1.0f  },
	{ .r = 0.75f, .g = 0.5f,  .b = 1.0f  },
	{ .r = 1.0f,  .g = 0.5f,  .b = 1.0f  },
	{ .r = 1.0f,  .g = 0.5f,  .b = 0.75f }
};

static void ResetParticle(Particle* particle)
{
	particle->position = (Vec3f){ .x = 0.0f, .y = 0.0f, .z = 0.0f };
	particle->velocity = (Vec3f)
	{
		.x = 10.0f * ((float)(NeHe_Random() % 50) - 26.0f),
		.y = 10.0f * ((float)(NeHe_Random() % 50) - 25.0f),
		.z = 10.0f * ((float)(NeHe_Random() % 50) - 25.0f)
	};
}

static void ParticlesInit(void)
{
	system.constant = (Vec2f){ .x = 0.0f, .y =  0.0f };
	system.gravity  = (Vec2f){ .x = 0.0f, .y = -0.8f };

	system.slowDown   = 2.0f;
	system.cycleDelay = 0;
	system.colorIndex = 0;

	system.autoCycle = true;

	for (unsigned i = 0; i < MAX_PARTICLES; ++i)
	{
		Particle* particle = &system.particles[i];

		particle->life  = 1.0f;
		particle->decay = 0.003f + (float)(NeHe_Random() % 100) / 1000.0f;
		particle->color = particleColors[0];  // Select red
		ResetParticle(particle);
	}
}

static void ParticlesUpdate(void)
{
	const float velocityScale = 0.001f / system.slowDown;

	for (unsigned i = 0; i < MAX_PARTICLES; ++i)
	{
		Particle* particle = &system.particles[i];

		particle->life -= particle->decay;
		if (particle->life < 0.0f)
		{
			particle->life     = 1.0f;
			particle->decay    = 0.003f + (float)(NeHe_Random() % 100) / 1000.0f;
			particle->color    = particleColors[system.colorIndex];
			particle->position = (Vec3f){ .x = 0.0f, .y = 0.0f, .z = 0.0f };
			particle->velocity = (Vec3f)
			{
				.x = (float)(NeHe_Random() % 60) - 32.0f + system.constant.x,
				.y = (float)(NeHe_Random() % 60) - 30.0f + system.constant.y,
				.z = (float)(NeHe_Random() % 60) - 30.0f
			};
		}
		else
		{
			// Integrate velocity
			particle->position.x += particle->velocity.x * velocityScale;
			particle->position.y += particle->velocity.y * velocityScale;
			particle->position.z += particle->velocity.z * velocityScale;

			// Integrate gravity
			particle->velocity.x += system.gravity.x;
			particle->velocity.y += system.gravity.y;
		}
	}

	// Cycle colours array
	if (system.autoCycle && system.cycleDelay > 25)
	{
		system.cycleDelay = 0;
		system.colorIndex = (system.colorIndex + 1) % SDL_arraysize(particleColors);
	}
	++system.cycleDelay;
}


typedef struct
{
	Vec4f position;
	SDL_FColor color;
} Instance;

static SDL_GPUGraphicsPipeline* pso = NULL;
static SDL_GPUTexture* particleTexture = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTransferBuffer* particleInstancesXferBuffer = NULL;
static SDL_GPUBuffer* particleInstancesGPUBuffer = NULL;

static Mtx projection;

static float zoom = -40.0f;


static bool Lesson19_Init(NeHeContext* restrict ctx)
{
	SDL_GPUShader* vertexShader = NULL, * fragmentShader = NULL;
	if (!NeHe_LoadShaders(ctx, &vertexShader, &fragmentShader, "lesson19",
		&(NeHeShaderProgramCreateInfo){ .vertexUniforms = 1, .fragmentSamplers = 1 }))
	{
		return false;
	}

	const SDL_GPUVertexAttribute vertexAttribs[] =
	{
		{
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = (Uint32)offsetof(Instance, position)
		},
		{
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = (Uint32)offsetof(Instance, color)
		}
	};
	pso = SDL_CreateGPUGraphicsPipeline(ctx->device, &(const SDL_GPUGraphicsPipelineCreateInfo)
	{
		.vertex_shader = vertexShader,
		.fragment_shader = fragmentShader,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_input_state =
		{
			.vertex_attributes = vertexAttribs,
			.num_vertex_attributes = SDL_arraysize(vertexAttribs),
			.vertex_buffer_descriptions = &(const SDL_GPUVertexBufferDescription)
			{
				.slot = 0,
				.pitch = sizeof(Instance),
				.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE
			},
			.num_vertex_buffers = 1
		},
		.rasterizer_state =
		{
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,  // Right-handed coordinates
			.enable_depth_clip = true  // OpenGL-like clip behaviour
		},
		.target_info =
		{
			.color_target_descriptions = &(const SDL_GPUColorTargetDescription)
			{
				.format = SDL_GetGPUSwapchainTextureFormat(ctx->device, ctx->window),
				.blend_state =
				{
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.enable_blend = true
				}
			},
			.num_color_targets = 1
		}
	});
	SDL_ReleaseGPUShader(ctx->device, fragmentShader);
	SDL_ReleaseGPUShader(ctx->device, vertexShader);
	if (!pso)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
		return false;
	}

	if ((particleTexture = NeHe_LoadTexture(ctx, "Data/Particle.bmp", true, false)) == NULL)
	{
		return false;
	}

	if ((sampler = SDL_CreateGPUSampler(ctx->device, &(const SDL_GPUSamplerCreateInfo)
	{
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR
	})) == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_GPUSamplerCreateInfo: %s", SDL_GetError());
		return false;
	}

	if ((particleInstancesXferBuffer = SDL_CreateGPUTransferBuffer(ctx->device, &(const SDL_GPUTransferBufferCreateInfo)
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(Instance) * MAX_PARTICLES
	})) == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUTransferBuffer: %s", SDL_GetError());
		return false;
	}

	if ((particleInstancesGPUBuffer = SDL_CreateGPUBuffer(ctx->device, &(const SDL_GPUBufferCreateInfo)
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(Instance) * MAX_PARTICLES
	})) == NULL)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateGPUBuffer: %s", SDL_GetError());
		return false;
	}

	ParticlesInit();

	return true;
}

static void Lesson19_Quit(NeHeContext* restrict ctx)
{
	SDL_ReleaseGPUBuffer(ctx->device, particleInstancesGPUBuffer);
	SDL_ReleaseGPUTransferBuffer(ctx->device, particleInstancesXferBuffer);
	SDL_ReleaseGPUSampler(ctx->device, sampler);
	SDL_ReleaseGPUTexture(ctx->device, particleTexture);
	SDL_ReleaseGPUGraphicsPipeline(ctx->device, pso);
}

static void Lesson19_Resize(NeHeContext* restrict ctx, int width, int height)
{
	(void)ctx;

	// Avoid division by zero by clamping height
	height = SDL_max(height, 1);
	// Recalculate projection matrix
	projection = Mtx_Perspective(45.0f, (float)width / (float)height, 0.1f, 200.0f);
}

static void Lesson19_Draw(NeHeContext* restrict ctx, SDL_GPUCommandBuffer* restrict cmd,
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

	// Fill instances buffer
	Instance* instances = (Instance*)SDL_MapGPUTransferBuffer(ctx->device, particleInstancesXferBuffer, true);
	unsigned numInstances = 0;
	for (unsigned i = 0; i < MAX_PARTICLES; ++i)
	{
		const Particle* particle = &system.particles[i];

		instances[numInstances++] = (Instance)
		{
			.position =
			{
				.x = particle->position.x,
				.y = particle->position.y,
				.z = particle->position.z,
				.w = 1.0f
			},
			.color =
			{
				.r = particle->color.r,
				.g = particle->color.g,
				.b = particle->color.b,
				.a = particle->life
			}
		};
	}
	SDL_UnmapGPUTransferBuffer(ctx->device, particleInstancesXferBuffer);

	// Upload instances to the GPU
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
	SDL_UploadToGPUBuffer(copyPass, &(const SDL_GPUTransferBufferLocation)
	{
		.transfer_buffer = particleInstancesXferBuffer
	}, &(const SDL_GPUBufferRegion)
	{
		.buffer = particleInstancesGPUBuffer,
		.size = sizeof(Instance) * numInstances
	}, true);
	SDL_EndGPUCopyPass(copyPass);

	// Begin render pass & bind pipeline state
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmd, &colorInfo, 1, NULL);
	SDL_BindGPUGraphicsPipeline(renderPass, pso);

	// Bind particle texture
	SDL_BindGPUFragmentSamplers(renderPass, 0, &(const SDL_GPUTextureSamplerBinding)
	{
		.texture = particleTexture,
		.sampler = sampler
	}, 1);

	// Bind particle instances buffer
	SDL_BindGPUVertexBuffers(renderPass, 0, &(const SDL_GPUBufferBinding)
	{
		.buffer = particleInstancesGPUBuffer,
		.offset = 0
	}, 1);

	// Push matrix uniform
	Mtx model = Mtx_Translation(0.0f, 0.0f, zoom);
	Mtx modelViewProjection = Mtx_Multiply(&projection, &model);
	SDL_PushGPUVertexUniformData(cmd, 0, &modelViewProjection, sizeof(modelViewProjection));

	// Draw particle instances
	SDL_DrawGPUPrimitives(renderPass, 4, numInstances, 0, 0);

	SDL_EndGPURenderPass(renderPass);

	ParticlesUpdate();

	const bool* keys = SDL_GetKeyboardState(NULL);

	// Adjust gravity with numpad arrows
	if ((keys[SDL_SCANCODE_KP_8] || keys[SDL_SCANCODE_I]) && system.gravity.y <  1.5f) { system.gravity.y += 0.01f; }
	if ((keys[SDL_SCANCODE_KP_2] || keys[SDL_SCANCODE_K]) && system.gravity.y > -1.5f) { system.gravity.y -= 0.01f; }
	if ((keys[SDL_SCANCODE_KP_6] || keys[SDL_SCANCODE_L]) && system.gravity.x <  1.5f) { system.gravity.x += 0.01f; }
	if ((keys[SDL_SCANCODE_KP_4] || keys[SDL_SCANCODE_J]) && system.gravity.x > -1.5f) { system.gravity.x -= 0.01f; }

	// Reset all particles with tab
	if (keys[SDL_SCANCODE_TAB]) { for (unsigned i = 0; i < MAX_PARTICLES; ++i) ResetParticle(&system.particles[i]); }

	// Adjust constant acceleration with arrow keys
	if (keys[SDL_SCANCODE_UP]    && system.constant.y <  200.0f) { system.constant.y += 1.0f; }
	if (keys[SDL_SCANCODE_DOWN]  && system.constant.y > -200.0f) { system.constant.y -= 1.0f; }
	if (keys[SDL_SCANCODE_RIGHT] && system.constant.x <  200.0f) { system.constant.x += 1.0f; }
	if (keys[SDL_SCANCODE_LEFT]  && system.constant.x > -200.0f) { system.constant.x -= 1.0f; }

	// Adjust speed with numpad -/+
	if ((keys[SDL_SCANCODE_KP_PLUS]  || keys[SDL_SCANCODE_EQUALS]) && system.slowDown > 1.0f) { system.slowDown -= 0.01f; }
	if ((keys[SDL_SCANCODE_KP_MINUS] || keys[SDL_SCANCODE_MINUS])  && system.slowDown < 4.0f) { system.slowDown += 0.01f; }

	// Adjust zoom wtih page up & page down
	if (keys[SDL_SCANCODE_PAGEUP])   { zoom += 0.1f; }
	if (keys[SDL_SCANCODE_PAGEDOWN]) { zoom -= 0.1f; }
}

static void Lesson19_Key(NeHeContext* ctx, SDL_Keycode key, bool down, bool repeat)
{
	(void)ctx;

	if (down && !repeat)
	{
		switch (key)
		{
		case SDLK_RETURN:
			// Toggle rainbow colour cycling
			system.autoCycle = !system.autoCycle;
			break;
		case SDLK_SPACE:
			// Disable rainbow cycling and advance colour manually
			system.autoCycle  = false;
			system.cycleDelay = 0;
			system.colorIndex = (system.colorIndex + 1) % SDL_arraysize(particleColors);
			break;
		default:
			break;
		}
	}
}


const struct AppConfig appConfig =
{
	.title = "NeHe's Particle Tutorial",
	.width = 640, .height = 480,
	.init = Lesson19_Init,
	.quit = Lesson19_Quit,
	.resize = Lesson19_Resize,
	.draw = Lesson19_Draw,
	.key = Lesson19_Key
};
