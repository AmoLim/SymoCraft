#pragma once
#include "symocraft/ecs/registry.h"
#include <string>

namespace SymoCraft::ECS::Detail {
    // Bounded legal-state fixtures; no native references or arbitrary mutation escape.
    struct RegistryTestAccess {
        static void PrepareNearCapacity(Registry&);
        static EntityId PrepareGeneration4093(Registry&);
        static void AdvanceIdentitySourceToLastBatch();
        static void SimulatePostClearConsistencyFailure(Registry&);
        static std::uint32_t FreshFrontier(const Registry&);
        template<class T> static std::string FirstName(const Registry& owner) {
            return Name(owner, &TypeToken<T>);
        }
    private:
        static std::string Name(const Registry&, const void*);
    };
}
