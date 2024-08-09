#pragma once

#include "ParticleGenerator.h"
#include <array>

struct alignas(16) StarburstData {
	struct alignas(16) TraceDirNumMods {
		float3 dir;
		uint32_t numAgeMods;
	};
	float3 partPos;
	int32_t missileAge;

	float3 partSpeed;
	uint32_t curTracerPart;

	int32_t drawOrder;
	int32_t unused[3];

	std::array<float4, 3> tracerPosSpeed;
	std::array<TraceDirNumMods, 3> tracerDir;
	std::array<float, 3 * 20> allAgeMods;

	AtlasedTexture texCoord1;
	AtlasedTexture texCoord3;

	int32_t GetMaxNumQuads() const {
		return
			3 * 20 * (texCoord1 != AtlasedTexture::DefaultAtlasTexture) +
			1 * (texCoord3 != AtlasedTexture::DefaultAtlasTexture);
	}
	void Invalidate() {
		texCoord1 = AtlasedTexture::DefaultAtlasTexture;
		texCoord3 = AtlasedTexture::DefaultAtlasTexture;
	}
};

static_assert(sizeof(StarburstData) % 16 == 0);

class StarburstParticleGenerator : public ParticleGenerator<StarburstData, StarburstParticleGenerator> {
public:
	StarburstParticleGenerator() {}
	~StarburstParticleGenerator() override {}
protected:
	bool GenerateCPUImpl() override { return false; }
};