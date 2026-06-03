#pragma once
#include "../ecs/entity_manager.h"
struct Scene
{
	ecs::EntityManager entity_manager; 
	virtual void Init() = 0; 
	virtual std::vector<uint32_t> GetUsedAssets() = 0; 
	virtual void Update(float delta_time) = 0;
	virtual void Destroy() = 0; 
};