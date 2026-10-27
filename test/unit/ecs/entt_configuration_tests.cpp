#include <symocraft/ecs/detail/entt_config.h>
#include <cstdio>

int main() {
    std::printf("EnTT=%d.%d.%d MSVC=%d iterator=%d empty-page=%zu\n",
        ENTT_VERSION_MAJOR, ENTT_VERSION_MINOR, ENTT_VERSION_PATCH, _MSC_FULL_VER,
        _ITERATOR_DEBUG_LEVEL, entt::component_traits<SymoCraft::ECS::Detail::EmptyStorageCheck>::page_size);
}
