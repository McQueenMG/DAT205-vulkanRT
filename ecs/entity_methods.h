#pragma once
#include <ecs/entity.h>
#include <ecs/entity_manager.h>

namespace ecs
{
    template <typename T>
    T* Entity::GetComponent() const noexcept
    {
        return entity_manager.GetComponentForEntity<T>(index);
    };

    template <typename... Ts>
    std::tuple<Ts*...> Entity::GetComponents() const noexcept
    {
        return {GetComponent<Ts>()...};
    }

    template <typename T>
    bool Entity::HasComponents() const noexcept
    {
        return entity_manager.EntityHasComponent<T>(index);
    }

    template <typename T0, typename T1, typename... Ts>
    bool Entity::HasComponents() const noexcept
    {
        return HasComponents<T0>() && HasComponents<T1, Ts...>();
    }

    // This unreadable mess allows us to write entity.AddComponent<Person>("name")
    // and construct it in place in the entity manager list.
    template <typename T, typename... ConstructorArgs>
    void Entity::AddComponent(ConstructorArgs&&... args)
    {
        entity_manager.AddComponentToEntity<T>(index, std::forward<ConstructorArgs>(args)...);
    }

    template <typename T>
    void Entity::RemoveComponent()
    {
        entity_manager.RemoveComponentFromEntity<T>(index);
    }
}
