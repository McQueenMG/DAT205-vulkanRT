#include "entity_manager.h"
#include "entity.h"
#include <assert.h>

namespace ecs
{
    EntityManager::EntityManager()
    {
        void* raw_memory = operator new[](MAX_NUM_ENTITIES * sizeof(Entity));
        entities = static_cast<Entity*>(raw_memory);
        component_bitmasks = new uint64_t[MAX_NUM_ENTITIES];
        ClearAll();
        num_registered_components = 0; 
    }

    EntityManager::~EntityManager()
    {
        delete[] entities; 
        delete[] component_bitmasks; 
    }

    void EntityManager::ClearAll()
    {
        num_entities = 0;
        for (int i = 0; i < MAX_NUM_ENTITIES; ++i) { 
            new(&entities[i])Entity(*this, i); 
            component_bitmasks[i] = 0ull; 
        }
    }

    int EntityManager::GetNextFreeEntityIndex() {
        for (int i = 0; i < (int)MAX_NUM_ENTITIES; i++) {
            if (entities[i].identifier == -1) return i;
        }
        // HANDLE TOO MANY ENTITIES
        std::cout << "Too Many Entities. Freak Out.\n";
        exit(0);
    }

    Entity & EntityManager::Create()
    {
        int index = GetNextFreeEntityIndex();
        entities[index].identifier = Entity::num_entities_created++;
        num_entities += 1; 
        return entities[index];
    }

    void EntityManager::Remove(Entity & entity)
    {
        assert(entities[entity.index].identifier != -1);
        entities[entity.index].identifier = -1; 
        component_bitmasks[entity.index] = 0; 
        num_entities -= 1; 
    }
}
