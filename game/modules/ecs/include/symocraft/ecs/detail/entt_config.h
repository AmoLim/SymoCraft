#pragma once

#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
#error SYMOCRAFT_ENTT_CONFIG: DBG01 requires uniform iterator level 0
#endif

#if !defined(ENTT_NO_ETO)
#error SYMOCRAFT_ENTT_CONFIG: use the configured ECS target (ENTT_NO_ETO required)
#endif
#if __has_include(<entt/ext/config.h>)
#error SYMOCRAFT_ENTT_CONFIG: external EnTT configuration is not supported
#endif
#if defined(ENTT_NO_EXCEPTION) || defined(ENTT_STANDARD_CPP) || defined(ENTT_USE_ATOMIC) || defined(ENTT_NO_MIXIN) || defined(ENTT_USE_STL) || defined(ENTT_FORCE_STL)
#error SYMOCRAFT_ENTT_CONFIG: unsupported EnTT configuration override
#endif

#include <entt/entity/registry.hpp>
#include <cstdint>
#include <type_traits>

static_assert(ENTT_VERSION_MAJOR == 4 && ENTT_VERSION_MINOR == 0 && ENTT_VERSION_PATCH == 0,
              "SYMOCRAFT_ENTT_CONFIG: fixed EnTT v4.0.0 required");
static_assert(ENTT_SPARSE_PAGE == 4096 && ENTT_PACKED_PAGE == 1024,
              "SYMOCRAFT_ENTT_CONFIG: page configuration changed");
static_assert(std::is_same_v<entt::id_type, std::uint32_t> && sizeof(entt::entity) == 4,
              "SYMOCRAFT_ENTT_CONFIG: default 32-bit identity types required");
static_assert(entt::entt_traits<entt::entity>::entity_mask == 0xFFFFF &&
              entt::entt_traits<entt::entity>::version_mask == 0xFFF,
              "SYMOCRAFT_ENTT_CONFIG: entity traits changed");
#if defined(_MSC_VER)
static_assert(_ITERATOR_DEBUG_LEVEL == 0, "SYMOCRAFT_ENTT_CONFIG: DBG01 requires uniform iterator level 0");
#pragma detect_mismatch("symocraft.entt.eto", "objects")
#endif

namespace SymoCraft::ECS::Detail {
    struct EmptyStorageCheck {};
    static_assert(entt::component_traits<EmptyStorageCheck>::page_size != 0,
                  "SYMOCRAFT_ENTT_CONFIG: empty components must have real objects");
}
