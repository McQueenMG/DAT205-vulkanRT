#pragma once
#include <stdint.h>
#include <tuple>

namespace ecs
{
    struct EntityManager;
    struct Entity
    {
        Entity(EntityManager & entity_manager, int index);
        EntityManager & entity_manager;
        const int index; // The index in the entity_manager list, so we don't have to search
        int identifier; // A unique identifier for this entity
        // This keeps track of the identifier, note that it is static to the _class_
        // on purpose, instead of just using entity_manager.num_entities, so that
        // entities can be differentiated between entity managers.  
        static int num_entities_created; 
        
        Entity& operator=(Entity& rhs) { return rhs; }; 

        bool operator == (const Entity & e) 
        {
            return (e.identifier == identifier);
        }

        bool operator == (const Entity & e) const 
        {
            return (e.identifier == identifier);
        }

        template <typename T>
        T* GetComponent() const noexcept;

        template <typename... Ts>
        std::tuple<Ts*...> GetComponents() const noexcept;

        template <typename T>
        bool HasComponents() const noexcept;

        template <typename T0, typename T1, typename... Ts>
        bool HasComponents() const noexcept;

        // This unreadable mess allows us to write entity.AddComponent<Person>("name")
        // and construct it in place in the entity manager list.
        template <typename T, typename... ConstructorArgs>
        void AddComponent(ConstructorArgs&&... args);

        template <typename T>
        void RemoveComponent();
    };
}
