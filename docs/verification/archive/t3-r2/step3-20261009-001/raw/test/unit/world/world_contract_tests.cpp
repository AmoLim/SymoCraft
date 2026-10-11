#include "world_test_access.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <type_traits>

namespace {
    using namespace SymoCraft;
    using namespace SymoCraft::World;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template<typename Exception, typename Callback> void Throws(Callback callback, const char* message)
    {
        bool caught = false;
        try { callback(); } catch (const Exception&) { caught = true; }
        Require(caught, message);
    }
    std::uint64_t Hash(MeshView vertices)
    {
        std::uint64_t value = 14695981039346656037ull;
        const auto* bytes = reinterpret_cast<const unsigned char*>(vertices.data());
        for (std::size_t i = 0; i < vertices.size_bytes(); ++i) { value ^= bytes[i]; value *= 1099511628211ull; }
        return value;
    }
    struct State {
        Revision content, input;
        std::optional<Revision> published;
        std::uint64_t mesh;
        bool operator==(const State&) const = default;
    };
    State StateOf(VoxelWorld& world, ChunkCoord coordinate)
    {
        const auto& chunk = WorldTestAccess::Chunk(world,coordinate);
        return {chunk.content_revision,chunk.mesh_input_revision,chunk.published_mesh_revision,Hash(chunk.mesh.vertices)};
    }
    std::vector<State> States(VoxelWorld& world)
    {
        std::vector<State> result;
        for (const auto& chunk : WorldTestAccess::Chunks(world)) result.push_back(StateOf(world,chunk->coordinate));
        return result;
    }
    std::unique_ptr<VoxelWorld> AirWorld(const BlockDefinition& definition)
    {
        auto world = VoxelWorld::Create(definition,{424242,2,false});
        WorldTestAccess::ResetToAir(*world);
        return world;
    }
    EditResult Set(VoxelWorld& world, BlockCoord position, BlockId id)
    { return world.TryEdit({EditOperation::Set,position,id}); }
    void CheckIsolation(const BlockDefinition& definition, const std::string& text)
    {
        auto first = AirWorld(definition);
        auto second_definition = BlockDefinition::FromConfig(text + "\ncustom_solid:\n  id: 12\n  side: 2\n  top: 2\n  bottom: 2\n  isSolid: true\n  isTransparent: false\n");
        auto second = AirWorld(second_definition);
        second_definition = definition;
        Require(first->Identity() != second->Identity(), "Two worlds reuse identity");
        Require(!first->DescribeBlock(12) && second->DescribeBlock(12)->m_is_solid, "World definition is shared or borrowed");
        const auto first_digest = first->Digest();
        const auto first_states = States(*first);
        Require(Set(*second,{8,200,8},12).status == EditStatus::Changed, "Second definition not bound to world");
        second->RebuildDirtyMeshes();
        Require(first->Digest() == first_digest && States(*first) == first_states, "Second world changed first blocks/versions/mesh");
        const auto second_digest = second->Digest();
        const auto second_states = States(*second);
        Require(Set(*first,{16,200,0},2).status == EditStatus::Changed, "First world edit failed");
        first->RebuildDirtyMeshes();
        Require(second->Digest() == second_digest && States(*second) == second_states, "First world adjacency reads another world");
        first.reset();
        Require(second->QueryBlock({8,200,8}).block.block_id == 12 && second->DescribeBlock(12), "Destroying one world invalidated another");
        const auto stable = second->Digest();
        for (int radius : {-1,0,1,11,std::numeric_limits<int>::max()})
            Throws<std::invalid_argument>([&] { static_cast<void>(VoxelWorld::Create(definition,{424242,radius,true})); }, "Invalid settings accepted");
        Require(second->Digest() == stable, "Failed factory modified existing world");
    }
    void CheckCoordinates(const BlockDefinition& definition)
    {
        auto world = AirWorld(definition);
        for (int x : {-17,-16,-1,0,15,16}) {
            Require(TryToBlockCoord({x + 0.75f,200.25f,-0.25f}) == BlockCoord(x,200,-1), "Float mapping does not use floor");
            Require(Set(*world,{x,200,3},2).status == EditStatus::Changed, "Valid chunk boundary rejected");
            Require(world->QueryBlock({x,200,3}).block.block_id == 2 && WorldTestAccess::RawBlock(*world,{x,200,3}).block_id == 2, "Chunk indexing crossed wrong neighbor");
        }
        for (int y : {0,255}) Require(Set(*world,{8,y,8},2).status == EditStatus::Changed, "Valid vertical extreme rejected");
        for (int y : {-1,256}) Require(world->QueryBlock({8,y,8}).status == BlockQueryStatus::OutsideWorld, "Vertical outside became air");
        Require(world->QueryBlock({-32,200,-32}).status == BlockQueryStatus::Found &&
            world->QueryBlock({47,200,47}).status == BlockQueryStatus::Found, "Fringe blocks not stored");
        Require(world->QueryBlock({-33,200,0}).status == BlockQueryStatus::OutsideWorld &&
            world->QueryBlock({48,200,0}).status == BlockQueryStatus::OutsideWorld, "World outside became stored air");
        const auto nan = std::numeric_limits<float>::quiet_NaN();
        const auto inf = std::numeric_limits<float>::infinity();
        for (const auto position : {glm::vec3(nan,0,0),glm::vec3(0,inf,0),glm::vec3(0,0,-inf),glm::vec3(2147483648.0f,0,0),glm::vec3(-2147483904.0f,0,0)})
            Require(!TryToBlockCoord(position), "Non-finite or i32-outside coordinate accepted");
        Require(TryToBlockCoord({-2147483648.0f,0,0})->x == std::numeric_limits<int>::min(), "i32 lower endpoint rejected");
        const auto upper = std::nextafter(2147483648.0f,0.0f);
        Require(TryToBlockCoord({upper,0,0})->x == 2147483520, "Representable i32 float below upper endpoint rejected");
        auto corner = AirWorld(definition);
        const auto diagonal = StateOf(*corner,{1,1});
        Require(Set(*corner,{15,200,15},2).status == EditStatus::Changed, "Four-chunk corner edit rejected");
        Require(StateOf(*corner,{0,0}).content == 2 && StateOf(*corner,{0,0}).input == 2 &&
            StateOf(*corner,{1,0}).content == 1 && StateOf(*corner,{1,0}).input == 2 &&
            StateOf(*corner,{0,1}).content == 1 && StateOf(*corner,{0,1}).input == 2 &&
            StateOf(*corner,{1,1}) == diagonal, "Corner invalidation includes diagonal or misses face neighbor");
    }
    void CheckEditsAndPublication(const BlockDefinition& definition)
    {
        auto world = AirWorld(definition);
        std::size_t visits = 0;
        world->VisitMeshes([&](const auto&) { ++visits; });
        Require(visits == 0, "Unpublished initial chunks visited as empty publication");
        const auto initial = States(*world);
        Require(Set(*world,{8,200,8},1).status == EditStatus::Unchanged && States(*world) == initial, "Unchanged cleared initial dirty state or advanced versions");
        for (const auto request : {EditRequest{EditOperation::Set,{8,200,8},0},EditRequest{EditOperation::Set,{8,200,8},65535},EditRequest{EditOperation::Remove,{100,200,0},0}})
            Require(world->TryEdit(request).status == EditStatus::Rejected && States(*world) == initial, "Rejected edit changed versions or mesh");
        Require(world->RebuildDirtyMeshes() == 9, "Initial empty publications not counted");
        world->VisitMeshes([&](const WorldMeshRecord& record) {
            Require(record.identity.world == world->Identity() && record.revision == 1 && record.mesh_input_revision == 1 && record.vertices.empty(), "Initial empty publication has wrong identity/version");
            ++visits;
        });
        Require(visits == 9 && world->RebuildDirtyMeshes() == 0, "Empty publication absent or rebuilt without edits");
        const MeshIdentity identity{world->Identity(),{0,0}};
        Require(Set(*world,{8,200,8},2).status == EditStatus::Changed, "Real edit not Changed");
        const auto dirty = StateOf(*world,{0,0});
        Require(dirty.content == 2 && dirty.input == 2 && dirty.published == 1, "Content/input/published versions conflated");
        Require(Set(*world,{8,200,8},2).status == EditStatus::Unchanged && StateOf(*world,{0,0}) == dirty, "Unchanged failed to preserve old publication and dirty input");
        Require(world->RebuildDirtyMeshes() == 1, "Interior edit rebuilt unrelated chunks");
        world->VisitMeshes([&](const WorldMeshRecord& record) {
            if (record.identity == identity) Require(record.revision == 2 && record.vertices.size() == 36 && record.vertices.size_bytes() == 36 * 28, "Real publication format/version changed");
        });
        auto& raw = WorldTestAccess::RawBlock(*world,{8,200,8});
        raw.SetTransparency(true);
        raw.lightLevel = 1234; raw.lightColor = 321; raw.bitwise_compressed_data |= 0x8000;
        Require(Set(*world,{8,200,8},2).status == EditStatus::Changed, "Same ID material correction wrongly Unchanged");
        const auto fixed = world->QueryBlock({8,200,8}).block;
        Require(!fixed.IsTransparent() && fixed.lightLevel == 1234 && fixed.lightColor == 321 && (fixed.bitwise_compressed_data & 0x8000), "Set reset untouched light/high flags");
        Require(world->TryEdit({EditOperation::Remove,{8,200,8}}).status == EditStatus::Changed, "Remove not Changed");
        const auto removed = world->QueryBlock({8,200,8}).block;
        Require(removed.block_id == 1 && removed.IsTransparent() && !removed.IsBlendable() && !removed.IsLightSource() && removed.lightLevel == 1234, "Remove differs from legacy affected-field policy");
        Require(world->RebuildDirtyMeshes() == 1, "Empty replacement not counted");
        world->VisitMeshes([&](const WorldMeshRecord& record) { if (record.identity == identity) Require(record.vertices.empty(), "Empty replacement left stale vertices"); });
    }
    struct Fault {
        ChunkCoord coordinate;
        Detail::MeshBuildStage stage;
        bool invoked = false;
    };
    void FailBuild(void* context, ChunkCoord coordinate, Detail::MeshBuildStage stage)
    {
        auto& fault = *static_cast<Fault*>(context);
        if (coordinate == fault.coordinate && stage == fault.stage) { fault.invoked = true; throw std::bad_alloc(); }
    }
    void InvalidCandidate(void*, MeshData& candidate) { candidate.vertices.push_back({}); }
    void CheckBuildFailuresAndReuse(const BlockDefinition& definition)
    {
        for (const auto stage : {Detail::MeshBuildStage::BeforeScratch,Detail::MeshBuildStage::BeforeOutput,Detail::MeshBuildStage::BeforeValidate}) {
            auto world = AirWorld(definition);
            for (const auto position : {BlockCoord{-8,200,-8},BlockCoord{8,200,8},BlockCoord{24,200,24}}) Set(*world,position,2);
            world->RebuildDirtyMeshes();
            const auto old_a = StateOf(*world,{-1,-1}), old_b = StateOf(*world,{0,0}), old_c = StateOf(*world,{1,1});
            for (const auto position : {BlockCoord{-7,200,-8},BlockCoord{9,200,8},BlockCoord{25,200,24}}) Set(*world,position,2);
            Fault fault{{0,0},stage};
            WorldTestAccess::Probe(*world,FailBuild,&fault);
            Throws<std::bad_alloc>([&] { world->RebuildDirtyMeshes(); }, "Build stage failure was swallowed");
            Require(fault.invoked, "Requested fault stage was not reached");
            const auto failed_a = StateOf(*world,{-1,-1}), failed_b = StateOf(*world,{0,0}), failed_c = StateOf(*world,{1,1});
            Require(failed_a.published == failed_a.input && failed_a.mesh != old_a.mesh, "Earlier success rolled back or failed to publish");
            Require(failed_b.published == old_b.published && failed_b.mesh == old_b.mesh && failed_b.input > *failed_b.published, "Failed block lost old publication or dirty state");
            Require(failed_c.published == old_c.published && failed_c.mesh == old_c.mesh && failed_c.input > *failed_c.published, "First-error policy processed later block");
            WorldTestAccess::Probe(*world,nullptr,nullptr);
            Require(world->RebuildDirtyMeshes() == 2, "Retry failed to publish remaining blocks");
            Require(world->RebuildDirtyMeshes() == 0, "Retry left residual dirty state");
        }
        auto world = AirWorld(definition);
        Set(*world,{8,200,8},2); world->RebuildDirtyMeshes();
        const auto owned_a = StateOf(*world,{0,0});
        const auto before = WorldTestAccess::Statistics(*world);
        Set(*world,{24,200,24},2); world->RebuildDirtyMeshes();
        Require(StateOf(*world,{0,0}) == owned_a, "Mesher B overwrote independently owned mesh A");
        world->TryEdit({EditOperation::Remove,{8,200,8}}); Set(*world,{8,200,8},2); world->RebuildDirtyMeshes();
        const auto after = WorldTestAccess::Statistics(*world);
        Require(after.scratch_capacity_bytes > 0 && after.scratch_growths == before.scratch_growths && after.builds > before.builds, "A-B-A did not reuse real scratch capacity");
        Require(StateOf(*world,{0,0}).mesh == owned_a.mesh, "A-B-A changed complete output ownership or order");
        const auto old = StateOf(*world,{0,0});
        Set(*world,{9,200,8},2);
        WorldTestAccess::CandidateProbe(*world,InvalidCandidate);
        Throws<std::length_error>([&] { world->RebuildDirtyMeshes(); }, "Actual invalid candidate size was published");
        Require(StateOf(*world,{0,0}).published == old.published && StateOf(*world,{0,0}).mesh == old.mesh, "Size validation destroyed old mesh");
        WorldTestAccess::CandidateProbe(*world,nullptr);
        Require(world->RebuildDirtyMeshes() == 1, "Actual size validation failure left mesher unusable");
        std::cout << "scratch_capacity_bytes=" << after.scratch_capacity_bytes << "; scratch_growths=" << after.scratch_growths
            << "; A-B-A builds=" << after.builds - before.builds << '\n';
    }
    void CheckRevisionExhaustion(const BlockDefinition& definition)
    {
        const auto max = std::numeric_limits<Revision>::max();
        for (int failure = 0; failure < 4; ++failure) {
            auto world = AirWorld(definition);
            auto& own = WorldTestAccess::Chunk(*world,{0,0});
            if (failure == 0) own.content_revision = max;
            if (failure == 1) own.mesh_input_revision = max;
            if (failure == 2) WorldTestAccess::Chunk(*world,{1,0}).mesh_input_revision = max;
            if (failure == 3) WorldTestAccess::Chunk(*world,{0,1}).mesh_input_revision = max;
            const auto before = States(*world);
            const auto digest = world->Digest();
            Throws<std::overflow_error>([&] { Set(*world,{15,200,15},2); }, "Revision exhaustion wrapped or committed");
            Require(States(*world) == before && world->Digest() == digest, "Exhaustion partially committed content or affected versions");
            Require(Set(*world,{15,200,15},1).status == EditStatus::Unchanged, "Exhausted but unchanged edit wrongly requires advance");
        }
        auto world = AirWorld(definition);
        auto& chunk = WorldTestAccess::Chunk(*world,{0,0});
        chunk.content_revision = chunk.mesh_input_revision = max - 1;
        Require(Set(*world,{8,200,8},2).status == EditStatus::Changed && chunk.content_revision == max && chunk.mesh_input_revision == max, "Last representable revision cannot commit");
        world->RebuildDirtyMeshes();
        Require(chunk.published_mesh_revision == max, "Max input publication was mistaken for overflow");
        const auto before = StateOf(*world,{0,0});
        Throws<std::overflow_error>([&] { Set(*world,{9,200,8},2); }, "Next revision wrapped");
        Require(StateOf(*world,{0,0}) == before, "Post-max rejection changed publication");
    }
    void CheckCallbacks(const BlockDefinition& definition)
    {
        auto world = AirWorld(definition);
        auto other = AirWorld(definition);
        world->RebuildDirtyMeshes();
        const auto before = States(*world);
        world->VisitMeshes([&](const WorldMeshRecord&) {
            Throws<std::logic_error>([&] { Set(*world,{8,200,8},2); }, "Visit mutation accepted");
            Throws<std::logic_error>([&] { world->RebuildDirtyMeshes(); }, "Visit rebuild accepted");
            Throws<std::logic_error>([&] { world->VisitMeshes([](const auto&) {}); }, "Nested Visit accepted");
            Require(world->QueryBlock({8,200,8}).status == BlockQueryStatus::Found, "Read during callback blocked");
            Require(Set(*other,{8,200,8},2).Accepted(), "Visit guard is shared between worlds");
        });
        Require(States(*world) == before, "Rejected reentry mutated world");
        Throws<std::runtime_error>([&] { world->VisitMeshes([](const auto&) { throw std::runtime_error("visitor failure"); }); }, "Visitor exception swallowed");
        std::size_t count = 0;
        world->VisitMeshes([&](const auto&) { ++count; });
        Require(count == 9 && Set(*world,{8,200,8},2).Accepted() && world->RebuildDirtyMeshes() == 1, "Throwing visitor left guard locked");
    }
}

int main(int argc, char** argv)
{
    using namespace SymoCraft;
    static_assert(sizeof(BlockVertex3D) == 28);
    static_assert(std::is_copy_constructible_v<World::BlockDefinition> && std::is_nothrow_move_constructible_v<World::BlockDefinition>);
    static_assert(!std::is_copy_constructible_v<World::VoxelWorld> && !std::is_copy_assignable_v<World::VoxelWorld> &&
        !std::is_move_constructible_v<World::VoxelWorld> && !std::is_move_assignable_v<World::VoxelWorld>);
    static_assert(!std::is_copy_constructible_v<World::Detail::ChunkMesher> && !std::is_copy_assignable_v<World::Detail::ChunkMesher> &&
        !std::is_move_constructible_v<World::Detail::ChunkMesher> && !std::is_move_assignable_v<World::Detail::ChunkMesher>);
    static_assert(!std::is_copy_constructible_v<Generation::Generator> && !std::is_copy_assignable_v<Generation::Generator> &&
        !std::is_move_constructible_v<Generation::Generator> && !std::is_move_assignable_v<Generation::Generator>);
    static_assert(std::is_copy_constructible_v<World::BlockDefinitionEntry> && std::is_copy_constructible_v<World::EditResult>);
    try {
        Require(argc == 2,"Expected real configuration path");
        std::ifstream input(argv[1],std::ios::binary);
        Require(static_cast<bool>(input),"Cannot read configuration");
        const std::string text{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto definition = World::BlockDefinition::FromConfig(text);
        CheckIsolation(definition,text);
        CheckCoordinates(definition);
        CheckEditsAndPublication(definition);
        CheckBuildFailuresAndReuse(definition);
        CheckRevisionExhaustion(definition);
        CheckCallbacks(definition);
        std::cout << "World isolation, integer edits, publication, failure recovery, scratch reuse and version/visit contracts passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
