//* ecs
//      entity component system implementation
//      heavily influenced by the design @skypjack [entt](https://github.com/skypjack/entt),
//      along with their helpful blog posts, [ecs back and forth](https://skypjack.github.io/2019-02-14-ecs-baf-part-1)
//      also would like to highlight @kgorking beautiful implementation of an [ecs](https://github.com/kgorking/ecs)
//      and david colson's amazing [post](https://www.david-colson.com/2020/02/09/making-a-simple-ecs.html)
#pragma once

#include "std_types.h"
#include "type_name.h"
#include "log.h"
#include <deque>

namespace fresa::ecs
{
    //* index-version id
    //      this is a numerical handle composed of a version and an index, being the version the first 16 bits and the index the last 16 bits
    //      the index is the entity handle, while the version exists to reuse deleted entity ids
    //      the id meant to be aliased for the types that require it, such as the entity and sparse set
    namespace detail 
    {
        using ID = strong::Type<ui32, decltype([]{}), strong::Regular, strong::Bitwise, strong::BitwiseWith<int>>;
    }
    using Index = strong::Type<ui16, decltype([]{}), strong::Regular, strong::ConvertibleTo<detail::ID>, strong::Hashable>;
    using Version = strong::Type<ui16, decltype([]{}), strong::Regular, strong::ConvertibleTo<detail::ID>, strong::Ordered, strong::Arithmetic>;

    [[nodiscard]] constexpr Index index(detail::ID id) noexcept { return Index(id.value); }
    [[nodiscard]] constexpr Version version(detail::ID id) noexcept { return Version((id >> 16).value); }
    [[nodiscard]] constexpr detail::ID id(Index i, Version v) noexcept { return (detail::ID(v) << 16) | detail::ID(i); }

    constexpr detail::ID invalid_id = id(-1, 0);

    //: alias for entities
    using EntityID = detail::ID;

    //---

    //* component pool
    //      a component pool is an allocator that stores components of a certain type
    //      the pool base is used to store references to the component pool in a hashed type map for later access
    //      the pool is a sparse set that stores the components using a dense array

    namespace detail
    {
        //: base component pool
        struct ComponentPoolBase {
            //: default constructor, no copy or move
            ComponentPoolBase() = default;
            ComponentPoolBase(const ComponentPoolBase&) = delete;
            ComponentPoolBase& operator=(const ComponentPoolBase&) = delete;
            ComponentPoolBase(ComponentPoolBase&&) = delete;
            ComponentPoolBase& operator=(ComponentPoolBase&&) = delete;
            virtual ~ComponentPoolBase() = default;

            //: alias for sparse set
            using SparseID = detail::ID;

            //: main array pair, sparse and dense
            std::unordered_map<Index, std::array<SparseID, engine_config.ecs_page_size()>> sparse;
            std::vector<Index> dense;

            //: get sparse
            //      gets the entity index and sees if it is included in the sparse array
            [[nodiscard]] constexpr const SparseID* sparse_at(const EntityID entity) const {
                const auto pos = index(entity).value;
                const auto page = pos / engine_config.ecs_page_size();
                return sparse.contains(page) ? &(sparse.at(page).at(pos % engine_config.ecs_page_size())) : nullptr;
            }
            [[nodiscard]] constexpr SparseID* sparse_at(const EntityID entity) {
                const auto pos = index(entity).value;
                const auto page = pos / engine_config.ecs_page_size();
                return sparse.contains(page) ? &(sparse.at(page).at(pos % engine_config.ecs_page_size())) : nullptr;
            }

            //: is valid
            //      checks if an sparse id handle points to something and has a propper version
            [[nodiscard]] constexpr bool valid(const SparseID* sid, const Version v) const {
                return sid != nullptr and *sid != invalid_id and version(*sid) == v;
            }

            //: contains entity
            //      checks if the entity is included in the sparse array, also verifying the version
            [[nodiscard]] constexpr bool contains(const EntityID entity) const {
                return valid(sparse_at(entity), version(entity));
            }
            
            //: remove is a constexpr virtual functions that is overriden by the derived classes
            constexpr virtual void remove(const EntityID entity) = 0;
        };
    }

    //: typed component pool
    template <typename T>
    struct ComponentPool : detail::ComponentPoolBase {
        //: data
        std::vector<T> data;

        //: add
        //      adds an entity to the sparse array, if there is an entity with a lower version it is updated
        //      if the entity has the same or higher version, an error is thrown
        constexpr void add(const EntityID entity, T&& value) {
            const auto pos = index(entity).value;
            const auto page = pos / engine_config.ecs_page_size();

            if (not sparse.contains(page))
                sparse[page].fill(invalid_id);

            auto& element = *sparse_at(pos);
            if (element == invalid_id) {
                element = id(dense.size(), version(entity));
                data.emplace_back(std::move(value));
                dense.emplace_back(index(entity));
            } else if (version(entity) > version(element)) {
                element = id(index(element), version(entity));
                data.at(index(element).value) = std::move(value);
                dense.at(index(element).value) = index(entity);
            } else {
                log::error("entity {} with version {} already exists in sparse set", entity.value, version(entity).value);
            }
        }

        //: get
        //      returns a pointer to the entity value from the dense array if it exists, if not it returns nullptr
        [[nodiscard]] constexpr const T* get(const EntityID entity) {
            const auto sid = sparse_at(entity);
            return valid(sid, version(entity)) ? &data.at(index(*sid).value) : nullptr;
        }

        //: remove
        //      removes an entity if it exists, otherwise it does nothing
        //      it swaps the last element with the removed element from both the sparse and dense arrays
        //      this function uses find if to find the last entity, however, it might be faster to use a dedicated extra array that holds
        //      the references to the entities from each spot of the dense array
        constexpr void remove(const EntityID entity) override {
            const auto sid = sparse_at(entity);
            if (not valid(sid, version(entity))) return;

            SparseID* last_sparse = sparse_at(dense.back().value);
            auto removed_element = sparse_at(index(entity).value);

            std::swap(*last_sparse, *removed_element);
            std::swap(dense.back(), dense.at(index(*last_sparse).value));
            std::swap(data.back(), data.at(index(*last_sparse).value));

            *removed_element = invalid_id;
            data.pop_back();
            dense.pop_back();
        }

        //: clear
        constexpr void clear() {
            log::info("clearing {}", type_name<T>());
            sparse.clear();
            data.clear();
        }

        //: size and extent
        [[nodiscard]] constexpr std::size_t size() const { return dense.size(); }
        [[nodiscard]] constexpr std::size_t extent() const { return sparse.size() * engine_config.ecs_page_size(); }

        //: iterators
        //- add support for dense entity iterators
        [[nodiscard]] constexpr auto begin() const noexcept { return data.begin(); }
        [[nodiscard]] constexpr auto end() const noexcept { return data.end(); }
        [[nodiscard]] constexpr auto cbegin() const noexcept { return data.cbegin(); }
        [[nodiscard]] constexpr auto cend() const noexcept { return data.cend(); }
        [[nodiscard]] constexpr auto rbegin() const noexcept { return data.rbegin(); }
        [[nodiscard]] constexpr auto rend() const noexcept { return data.rend(); }
        [[nodiscard]] constexpr auto crbegin() const noexcept { return data.crbegin(); }
        [[nodiscard]] constexpr auto crend() const noexcept { return data.crend(); }
    };

    //---

    //* scene
    //-     ...
    struct Scene {
        //* component pool
        //      searchs the hash map for the specified component type
        //      if none is found, create it. this operation is thread safe

        //: hash map of component pools, using the component type as a key
        std::unordered_map<TypeHash, std::unique_ptr<detail::ComponentPoolBase>> component_pools;

        //: this mutex prevents the creation of multiple component pools of the same type
        std::mutex component_pool_create_mutex;

        //: get component pool
        template <typename C>
        auto& cpool() {
            constexpr TypeHash t = type_hash<C>();
            std::lock_guard<std::mutex> lock(component_pool_create_mutex);

            //: find the pool using the type hash
            auto it = component_pools.find(t);

            //: there is no pool, create it
            if (it == component_pools.end()) {
                auto pool = std::make_unique<ComponentPool<C>>();
                it = component_pools.emplace(t, std::move(pool)).first;
            }

            //: return a reference to the pool
            return (ComponentPool<C>&)(*it->second);
        }
        template <typename C>
        const auto& cpool() const { return const_cast<Scene*>(this)->cpool<C>(); }

        // ---

        //* entities
        //-     ...

        std::deque<EntityID> free_entities = {ecs::id(0, 0)};

        //: add entity
        template <typename ... C>
        constexpr const EntityID add(C&& ... components) {
            const auto entity = free_entities.front();
            if (free_entities.size() > 1) free_entities.pop_front();
            else free_entities.back() = entity.value + 1;
            (cpool<C>().add(entity, std::forward<C>(components)), ...);
            return entity;
        }

        //: get entity component
        template <typename C>
        [[nodiscard]] constexpr const C* get(const EntityID entity) {
            return cpool<C>().get(entity);
        }

        //: remove entity
        constexpr void remove(const EntityID entity) {
            [&] { for (auto &[key, pool] : component_pools) pool->remove(entity); }();
            free_entities.push_front(id(index(entity), version(entity) + Version(1)));
        }
    };

    //* view
    //! for now one component only, not C..., also needs to include the entity id, maybe divide dense and data again? check if entity valid as well
    //! OK FINALLY, entt does it by using the dense array as data in the unspetialized array

    template <typename C>
    struct View {
        //: scene pointer
        Scene* scene;

        //: if a scene view doesn't specify any types, it will iterate over all of them
        static constexpr bool all = false;//sizeof...(C) == 0;

        //: constructor
        constexpr View(Scene& s) : scene(&s) {}

        //: iterator
        //      iterates over all entities that have the specified components
        //      if no components are specified, it will iterate over all entities
        [[nodiscard]] constexpr auto begin() const noexcept {
            return scene->cpool<C>().begin();
        }
        [[nodiscard]] constexpr auto end() const noexcept {
            return scene->cpool<C>().end();
        }
    };

    namespace detail
    {
        template <std::size_t N>
        struct ViewIterator {
            std::array<const ComponentPoolBase*, N> pools;


            [[nodiscard]] bool valid() const noexcept {
                
            }
        };
    }
}