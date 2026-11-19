#include "world_test_access.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <new>

namespace {
    thread_local bool fail_allocation = false;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template<class Exception, class Callback> void Throws(Callback callback) {
        try { callback(); } catch (const Exception&) { return; }
        throw std::runtime_error("Expected exception was not thrown");
    }
}
void* operator new(std::size_t bytes) {
    if (fail_allocation) throw std::bad_alloc();
    if (auto* result = std::malloc(bytes ? bytes : 1)) return result;
    throw std::bad_alloc();
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

int main(int argc, char** argv) {
    using namespace SymoCraft;
    using namespace SymoCraft::World;
    try {
        Require(argc == 2, "Expected block definition path");
        std::ifstream input(argv[1], std::ios::binary);
        const std::string base{std::istreambuf_iterator<char>(input), {}};
        const std::string entry = "\nresource_block:\n  id: 12\n  side: 2\n  top: 2\n  bottom: 2\n  isSolid: true\n  isTransparent: false\n  harvest: {seconds: 0.5, resource: wood, quantity: 2}\n";
        Require(!BlockDefinition::FromConfig(base).Find(6)->harvest, "Old definition gained a harvest rule");
        const auto definition = BlockDefinition::FromConfig(base + entry);
        const auto rule = definition.Find(12)->harvest;
        Require(rule && rule->base_seconds == 0.5 && rule->resource_id == "wood" && rule->quantity == 2,
                "Harvest rule fields differ");
        for (const auto bad : {"0", "-1", ".nan", ".inf"}) {
            auto invalid = entry;
            invalid.replace(invalid.find("seconds: 0.5"), 12, std::string("seconds: ") + bad);
            Throws<std::runtime_error>([&] { BlockDefinition::FromConfig(base + invalid); });
        }
        for (const auto bad : {"0", "-1", "18446744073709551616"}) {
            auto invalid = entry;
            invalid.replace(invalid.find("quantity: 2"), 11, std::string("quantity: ") + bad);
            Throws<std::runtime_error>([&] { BlockDefinition::FromConfig(base + invalid); });
        }
        auto world = VoxelWorld::Create(definition, {424242, 2, false}, nullptr, true);
        auto other = VoxelWorld::Create(definition, {424242, 2, false}, nullptr, true);
        const BlockCoord target{0, 200, 0};
        world->TryEdit({EditOperation::Set, target, 12});
        auto token = *world->CaptureHarvestTarget(target);
        Require(world->IsHarvestTargetValid(token) && !other->IsHarvestTargetValid(token), "World identity mismatch accepted");
        world->TryEdit({EditOperation::Set, {1, 200, 0}, 12});
        Require(world->TryEdit({EditOperation::Set, target, 12}).status == EditStatus::Unchanged &&
                world->IsHarvestTargetValid(token), "Unrelated/no-op edit invalidates original target");
        Require(world->TryHarvestRemove(token).status == EditStatus::Changed, "Conditional remove failed");
        Require(world->TryHarvestRemove(token).rejection == EditRejection::StaleTarget, "Duplicate remove accepted");
        world->TryEdit({EditOperation::Set, target, 12});
        Require(!world->IsHarvestTargetValid(token), "Same-type replacement resurrected target");
        token = *world->CaptureHarvestTarget(target);
        auto& chunk = WorldTestAccess::Chunk(*world, {0, 0});
        const auto content = chunk.content_revision;
        const auto mesh = chunk.mesh_input_revision;
        const BlockCoord fresh{2, 200, 0};
        bool failed = false;
        fail_allocation = true;
        try { world->TryEdit({EditOperation::Set, fresh, 12}); }
        catch (const std::bad_alloc&) { failed = true; }
        fail_allocation = false;
        Require(failed && world->QueryBlock(fresh).block.block_id == 1 &&
                chunk.content_revision == content && chunk.mesh_input_revision == mesh &&
                world->IsHarvestTargetValid(token), "Allocation failure partially committed cell versions");
        const auto old_revision = chunk.content_revision;
        chunk.content_revision = std::numeric_limits<Revision>::max();
        Throws<std::overflow_error>([&] { world->TryHarvestRemove(token); });
        Require(world->IsHarvestTargetValid(token) && chunk.mesh_input_revision == mesh, "World revision exhaustion changed target");
        chunk.content_revision = old_revision;
        WorldTestAccess::ExhaustHarvestRevision(*world, target);
        auto exhausted = *world->CaptureHarvestTarget(target);
        Throws<std::overflow_error>([&] { world->TryHarvestRemove(exhausted); });
        Require(world->IsHarvestTargetValid(exhausted) && chunk.content_revision == old_revision,
                "Single-cell revision exhaustion partially committed");
        std::cout << "Harvest rules, target identity, ABA, no-op and failure atomicity passed\n";
        return 0;
    } catch (const std::exception& error) {
        fail_allocation = false;
        std::cerr << error.what() << '\n';
        return 1;
    }
}
