#include "entity.h"
#include "entity_manager.h"

namespace ecs
{
    int Entity::num_entities_created = 0;
    // This is only called on initialization of the entity_manager
    Entity::Entity(EntityManager & entity_manager, int index) :
        entity_manager(entity_manager),
        identifier(-1),
        index(index) {}

}