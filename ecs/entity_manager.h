#pragma once
#include <array>
#include <vector>
#include <iostream>
#include <string>
#include <tuple>
#include <assert.h>
#include "entity.h"

namespace ecs
{
    ///////////////////////////////////////////////////////////////////////////
    // Maximum number of components is fixed so that one 64bit word is enough,
    // but maximum number of enities can be changed at will.
    ///////////////////////////////////////////////////////////////////////////
    constexpr int MAX_NUM_ENTITIES = 1000000;
    constexpr int MAX_NUM_COMPONENTS = 64;

    struct EntityManager
    {
        // NOTE: lists here are good ol' arrays for (verified)
        //       performance reasons.
        std::array<void *, MAX_NUM_COMPONENTS> component_arrays;
        Entity *entities;
        int num_entities = 0;
        uint64_t *component_bitmasks;
        EntityManager();
        ~EntityManager();

        int num_registered_components = 0; 

        template <typename T>
        void RegisterComponent()
        {
            T::component_type_id = num_registered_components;    
            component_arrays[num_registered_components++] =
                new T[MAX_NUM_ENTITIES];
        }

        void ClearAll();
        int GetNextFreeEntityIndex();
        template <typename T>
        T *GetComponentForEntity(int index)
        {
            if ((component_bitmasks[index] & (1ull << T::component_type_id)) == 0) return nullptr;
            T *components = (T *)component_arrays[T::component_type_id];
            return &components[index];
        }

        template <typename T>
        bool EntityHasComponent(int index)
        {
            assert(entities[index].index != -1);
            return (component_bitmasks[index] & (0x1ull << T::component_type_id)) != 0;
        }

        template <typename T, typename... ConstructorArgs>
        void AddComponentToEntity(int index, ConstructorArgs &&... args)
        {
            T *components = (T *)component_arrays[T::component_type_id];
            component_bitmasks[index] |= (1ull << T::component_type_id);
            new (&components[index]) T(std::forward<ConstructorArgs>(args)...);
        }
        template <typename T>
        void RemoveComponentFromEntity(int index)
        {
            // Just unsetting the bit. Data is not cleared.
            T *components = (T *)component_arrays[T::component_type_id];
            component_bitmasks[index] &= ~(1ull << T::component_type_id);
        }
        Entity &Create();
        void Remove(Entity &entity);

        template <typename... ComponentTypes>
        struct EntityIterator;
        template <typename... ComponentTypes>
        struct EntityEnumerator
        {
            using iterator = EntityIterator<ComponentTypes...>;
            using const_iterator = EntityIterator<const ComponentTypes...>;
            friend iterator;
            EntityEnumerator(const EntityManager &entity_manager) noexcept : entity_manager(entity_manager) {}
            inline iterator end() const noexcept
            {
                return iterator(*this, MAX_NUM_ENTITIES, entity_manager.num_entities);
            }
            inline iterator begin() const noexcept { return next(iterator(*this, -1, 0)); }
            // TODO: Implement these.
            inline iterator cend() noexcept = delete;
            inline iterator cbegin() noexcept = delete;

           private:
            // Methods
            inline Entity const *get(const iterator &it) const noexcept
            {
                return &(entity_manager.entities[it.entity_index]);
            }
            inline iterator next(const iterator &it) const noexcept
            {
                int entity_counter = it.entity_counter;
                for (int current_idx = it.entity_index + 1; entity_counter < entity_manager.num_entities; ++current_idx)
                {
                    const Entity &entity = entity_manager.entities[current_idx];
                    if (entity.identifier == -1) continue;
                    entity_counter += 1;
                    if (entity.HasComponents<ComponentTypes...>()) return iterator(*this, current_idx, entity_counter);
                }
                return end();
            }
            // State
            const EntityManager &entity_manager;
        };

        template <typename... ComponentTypes>
        struct EntityIterator
        {
            friend struct EntityEnumerator<ComponentTypes...>;
            // Custom constructor
            EntityIterator(const EntityEnumerator<ComponentTypes...> &entity_enumerator, int idx, int counter) noexcept
                : entity_index(idx), entity_enumerator(entity_enumerator), entity_counter(counter)
            {
            }

            // Needed for iterator
            EntityIterator(const EntityIterator &other) = default;
            EntityIterator &operator=(const EntityIterator &other) = default;
            ~EntityIterator() = default;
            inline Entity &operator*() const noexcept
            {
                // We have to do a const cast here as, while we do have a const ref to
                // entity_enumerator and this function itself do not modify any values,
                // we do not care about constness of the referece returned from this function.
                return *(const_cast<Entity *>(entity_enumerator.get(*this)));
            }
            inline EntityIterator &operator++() noexcept
            {
                auto iter = entity_enumerator.next(*this);
                entity_index = iter.entity_index;
                entity_counter = iter.entity_counter;
                return *this;
            }

            // Needed for input / output iterator
            inline EntityIterator &operator++(int) noexcept
            {
                EntityIterator tmp(*this);
                operator++();
                return tmp;
            }
            inline bool operator!=(const EntityIterator &other) const noexcept
            {
                return entity_index != other.entity_index;
            }
            inline bool operator==(const EntityIterator &other) const noexcept
            {
                return entity_index == other.entity_index;
            }

            // Needed for forward iterator (Must also be immutable to have const iterator)
            EntityIterator() = default;

           private:
            // State
            int entity_index = -1;
            int entity_counter = 0;
            const EntityEnumerator<ComponentTypes...> &entity_enumerator;
        };

        template <typename... ComponentTypes>
        EntityEnumerator<ComponentTypes...> EntitiesWithComponents() noexcept
        {
            return EntityEnumerator<ComponentTypes...>(*this);
        }

        // This could probably be much faster if necessary
        template <typename... ComponentTypes>
        int CountEntitiesWithComponents() noexcept
        {
            int ret = 0;
            for (Entity &entity : EntitiesWithComponents<ComponentTypes...>())
            {
                ret += 1;
            }
            return ret;
        }
    };
}  // namespace ecs
