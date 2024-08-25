/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FlameProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Env/Particles/Generators/ParticleGeneratorHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

#include "System/Misc/TracyDefs.h"

CR_BIND_DERIVED(CFlameProjectile, CWeaponProjectile, )

CR_REG_METADATA(CFlameProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(spread),
	CR_MEMBER(curTime),
	CR_MEMBER(physLife),
	CR_MEMBER(invttl),
	CR_MEMBER(pgOffset)
))


CFlameProjectile::CFlameProjectile(const ProjectileParams& params)
	: CWeaponProjectile(params)
	, curTime(0.0f)
	, physLife(0.0f)
	, invttl(1.0f / ttl)
	, spread(params.spread)
{
	projectileType = WEAPON_FLAME_PROJECTILE;

	SetRadiusAndHeight(weaponDef->size * weaponDef->collisionSize, 0.0f);

	drawRadius = weaponDef->size;
	physLife = 1.0f / weaponDef->duration;

	const WeaponDef::Visuals& wdVisuals = weaponDef->visuals;
	auto DefColor = SColor(wdVisuals.color.x, wdVisuals.color.y, wdVisuals.color.z, weaponDef->intensity);

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<FlameParticleGenerator>();
	pgOffset = pg.Add({
		.pos = {},
		.radius = radius,
		.animParams = {},
		.drawOrder = drawOrder,
		.rotParams = {},
		.curTime = curTime,
		.color0 = DefColor,
		.color1 = DefColor,
		.colEdge0 = 0.0f,
		.colEdge1 = 1.0f,
		.texCoord = *wdVisuals.texture1
	});
}

CFlameProjectile::~CFlameProjectile()
{
	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<FlameParticleGenerator>();
	pg.Del(pgOffset);
}

void CFlameProjectile::Collision()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3& norm = CGround::GetNormal(pos.x, pos.z);
	const float ns = speed.dot(norm);

	SetVelocityAndSpeed(speed - (norm * ns));
	SetPosition(pos + UpVector * 0.05f);

	curTime += 0.05f;
}

void CFlameProjectile::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!luaMoveCtrl) {
		SetPosition(pos + speed);
		UpdateGroundBounce();
		SetVelocityAndSpeed(speed + spread);
	}

	UpdateInterception();

	radius = radius + weaponDef->sizeGrowth;
	sqRadius = radius * radius;
	drawRadius = radius * weaponDef->collisionSize;

	curTime = std::min(curTime + invttl, 1.0f);
	checkCol &= (curTime <= physLife);
	deleteMe |= (curTime >= 1.0f);

	auto& pg = ParticleGeneratorHandler::GetInstance().GetGenerator<FlameParticleGenerator>();
	auto& data = pg.Get(pgOffset);

	data.radius = radius;
	data.pos = pos;
	data.curTime = curTime;

	if (const auto* cm = weaponDef->visuals.colorMap; cm) {
		auto [i0, i1] = cm->GetIndices(curTime);
		data.color0 = cm->GetColor(i0);
		data.color1 = cm->GetColor(i1);
		data.colEdge0 = cm->GetColorPos(i0);
		data.colEdge1 = cm->GetColorPos(i1);
	}

	explGenHandler.GenExplosion(cegID, pos, speed, curTime, 0.0f, 0.0f, owner(), nullptr);
}

void CFlameProjectile::Draw()
{
	/*
	RECOIL_DETAILED_TRACY_ZONE;
	if (!validTextures[0])
		return;

	unsigned char col[4];
	weaponDef->visuals.colorMap->GetColor(col, curTime);

	AddEffectsQuad(
		{ drawPos - camera->GetRight() * radius - camera->GetUp() * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col },
		{ drawPos + camera->GetRight() * radius - camera->GetUp() * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col },
		{ drawPos + camera->GetRight() * radius + camera->GetUp() * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col },
		{ drawPos - camera->GetRight() * radius + camera->GetUp() * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col }
	);
	*/
}

int CFlameProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) >= shieldMaxSpeed)
		return 0;

	SetVelocityAndSpeed(speed + (rdir * shieldForce));
	return 2;
}

int CFlameProjectile::GetProjectilesCount() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return 1 * validTextures[0];
}
