#include "batch.hpp"
#include <symocraft/world/world.h>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <iostream>
#include <stdexcept>

namespace {
    struct Calls {
        GLuint next_id{1};
        GLsizeiptr storage_size{};
        GLsizei stride{};
        GLsizei draw_count{};
        int uploads{};
        int draws{};
        int deleted_buffers{};
        int deleted_arrays{};
        bool fail_array_creation{};
        bool fail_storage{};
        std::vector<unsigned char> uploaded;
    } calls;

    void APIENTRY CreateBuffers(GLsizei count, GLuint* ids)
    {
        for (GLsizei i = 0; i < count; ++i)
            ids[i] = calls.next_id++;
    }
    void APIENTRY CreateArrays(GLsizei count, GLuint* ids)
    {
        for (GLsizei i = 0; i < count; ++i)
            ids[i] = calls.fail_array_creation ? 0 : calls.next_id++;
    }
    void APIENTRY Storage(GLuint, GLsizeiptr size, const void*, GLbitfield) { calls.storage_size = size; }
    void APIENTRY StorageSize(GLuint, GLenum, GLint64* size) { *size = calls.fail_storage ? 0 : calls.storage_size; }
    void APIENTRY VertexBuffer(GLuint, GLuint, GLuint, GLintptr, GLsizei stride) { calls.stride = stride; }
    void APIENTRY EnableAttribute(GLuint, GLuint) {}
    void APIENTRY AttributeFormat(GLuint, GLuint, GLint, GLenum, GLboolean, GLuint) {}
    void APIENTRY AttributeBinding(GLuint, GLuint, GLuint) {}
    void APIENTRY BindArray(GLuint) {}
    void APIENTRY Draw(GLenum, GLint, GLsizei count) { calls.draw_count = count; ++calls.draws; }
    void APIENTRY DeleteBuffers(GLsizei count, const GLuint*) { calls.deleted_buffers += count; }
    void APIENTRY DeleteArrays(GLsizei count, const GLuint*) { calls.deleted_arrays += count; }
    void APIENTRY Upload(GLuint, GLintptr offset, GLsizeiptr size, const void* vertices)
    {
        if (offset != 0 || size < 0 || size > calls.storage_size)
            throw std::runtime_error("Invalid buffer upload range.");
        const auto* bytes = static_cast<const unsigned char*>(vertices);
        calls.uploaded.assign(bytes, bytes + size);
        ++calls.uploads;
    }

    void InstallGLStubs()
    {
        glad_glCreateBuffers = CreateBuffers;
        glad_glCreateVertexArrays = CreateArrays;
        glad_glNamedBufferStorage = Storage;
        glad_glGetNamedBufferParameteri64v = StorageSize;
        glad_glVertexArrayVertexBuffer = VertexBuffer;
        glad_glEnableVertexArrayAttrib = EnableAttribute;
        glad_glVertexArrayAttribFormat = AttributeFormat;
        glad_glVertexArrayAttribBinding = AttributeBinding;
        glad_glBindVertexArray = BindArray;
        glad_glDrawArrays = Draw;
        glad_glDeleteBuffers = DeleteBuffers;
        glad_glDeleteVertexArrays = DeleteArrays;
        glad_glNamedBufferSubData = Upload;
    }

    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    template<class Exception, class Function>
    void ExpectThrow(Function&& function, const char* message)
    {
        try {
            function();
        } catch (const Exception&) {
            return;
        }
        throw std::runtime_error(message);
    }

    void CheckCapacityAndStride()
    {
        using namespace SymoCraft;
        using LineBatch = Batch<LineVertex3D>;
        constexpr auto maximum = (std::numeric_limits<std::size_t>::max)();
        static_assert(LineBatch::CanAppend(0, 2, 2));
        static_assert(!LineBatch::CanAppend(2, 1, 2));
        static_assert(!LineBatch::CanAppend(maximum - 1, 2, maximum));
        static_assert(!LineBatch::CanAppend(3, 0, 2));

        calls = {};
        LineBatch batch;
        const LineVertex3D first{{1.25f, 2.5f, 3.75f}};
        const LineVertex3D second{{4.25f, 5.5f, 6.75f}};
        ExpectThrow<std::logic_error>([&] { batch.AddVertex(first); }, "Uninitialized batch accepted data.");
        ExpectThrow<std::length_error>([&] { batch.SetBatchSize(0); }, "Zero batch capacity was accepted.");
        ExpectThrow<std::length_error>([&] { batch.SetBatchSize(maximum); }, "Unrepresentable batch capacity was accepted.");
        batch.SetBatchSize(2);
        batch.Init({{0, 3, GL_FLOAT, offsetof(LineVertex3D, pos_coord)}});
        Require(calls.stride == sizeof(LineVertex3D), "Line batch used another vertex type's stride.");
        Require(calls.storage_size == 2 * sizeof(LineVertex3D), "VBO storage byte size is incorrect.");
        batch.AddVertex(first);
        batch.AddVertex(&second, 1);
        batch.ReloadData();
        Require(calls.uploaded.size() == 2 * sizeof(LineVertex3D), "Upload did not use the live vertex byte count.");
        Require(std::memcmp(calls.uploaded.data(), &first, sizeof(first)) == 0, "Single append data was overwritten.");
        Require(std::memcmp(calls.uploaded.data() + sizeof(first), &second, sizeof(second)) == 0,
            "Bulk append data did not follow the first vertex.");
        ExpectThrow<std::length_error>([&] { batch.AddVertex(first); }, "Exact-full batch accepted another vertex.");
        Require(batch.VertexCount() == 2, "Rejected append changed the vertex count.");
        batch.Draw();
        Require(calls.draw_count == 2 && calls.uploads == 1, "Draw count or dirty upload tracking is incorrect.");
        Require(batch.VertexCount() == 0, "Draw did not clear the live count.");
        batch.Draw();
        Require(calls.draws == 1, "An empty batch issued another draw.");
        batch.AddVertex(nullptr, 0);
        ExpectThrow<std::invalid_argument>([&] { batch.AddVertex(nullptr, 1); }, "Nonempty null range was accepted.");
        batch.AddVertex(first);
        const std::array<LineVertex3D, 2> pair{first, second};
        ExpectThrow<std::length_error>([&] { batch.AddVertex(pair.data(), pair.size()); }, "Bulk append ignored its complete size.");
        Require(batch.VertexCount() == 1, "Rejected bulk append changed the live count.");
        ExpectThrow<std::logic_error>([&] { batch.SetBatchSize(4); }, "Live GPU storage capacity was changed without reinitialization.");
        batch.Free();
        batch.Free();
        Require(calls.deleted_buffers == 1 && calls.deleted_arrays == 1, "Repeated cleanup leaked or double-deleted GL objects.");
    }

    void CheckPartialInitialization()
    {
        using namespace SymoCraft;
        calls = {};
        calls.fail_array_creation = true;
        Batch<LineVertex3D> batch;
        batch.SetBatchSize(2);
        ExpectThrow<std::runtime_error>([&] { batch.Init({}); }, "VAO creation failure was ignored.");
        batch.Free();
        batch.Free();
        Require(calls.deleted_buffers == 1 && calls.deleted_arrays == 0, "Partial init cleanup deleted invalid objects.");

        calls = {};
        calls.fail_storage = true;
        ExpectThrow<std::runtime_error>([&] { batch.Init({}); }, "Buffer allocation failure was ignored.");
        batch.Free();
        Require(calls.deleted_buffers == 1 && calls.deleted_arrays == 1, "Failed storage cleanup leaked GL objects.");
    }

    void CheckEmptyWorldPublication(const char* configuration_path)
    {
        using namespace SymoCraft;
        std::ifstream input(configuration_path, std::ios::binary);
        Require(static_cast<bool>(input), "Cannot read real block configuration.");
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        auto world = World::VoxelWorld::Create(World::BlockDefinition::FromConfig(text), {424242, 2, false});
        Require(world->RebuildDirtyMeshes() == 9, "Playable initial world did not publish all nine chunks.");

        calls = {};
        Batch<BlockVertex3D> batch;
        batch.Init({{0, 3, GL_INT, offsetof(BlockVertex3D, pos_coord)},
                    {1, 3, GL_FLOAT, offsetof(BlockVertex3D, tex_coord)},
                    {2, 1, GL_FLOAT, offsetof(BlockVertex3D, normal)}});
        std::size_t vertices = 0;
        world->VisitMeshes([&](const World::WorldMeshRecord& record) {
            vertices += record.vertices.size();
            batch.AddVertex(record.vertices.data(), record.vertices.size());
        });
        Require(vertices > 0 && batch.VertexCount() == vertices, "Real published meshes did not reach the old batch.");
        batch.Draw();
        Require(calls.draws == 1 && calls.uploads == 1 &&
                static_cast<std::size_t>(calls.draw_count) == vertices && batch.VertexCount() == 0,
                "Nonempty publication did not draw and clear the live batch.");

        // Exercise only public world edits; no private air fixture bypasses publication.
        for (int x = -16; x < 32; ++x)
            for (int z = -16; z < 32; ++z)
                for (int y = 0; y < k_chunk_height; ++y) {
                    const World::BlockCoord coordinate{x, y, z};
                    const auto block = world->QueryBlock(coordinate);
                    Require(block.status == World::BlockQueryStatus::Found, "Playable cell became outside world.");
                    if (block.block.block_id != 1)
                        Require(world->TryEdit({World::EditOperation::Remove, coordinate}).Accepted(),
                                "Public removal failed while emptying the playable world.");
                }
        Require(world->RebuildDirtyMeshes() == 9, "Empty replacement did not publish all affected chunks.");
        const auto draws_before = calls.draws, uploads_before = calls.uploads;
        std::size_t empty_records = 0;
        world->VisitMeshes([&](const World::WorldMeshRecord& record) {
            Require(record.identity.world == world->Identity() && record.vertices.empty(),
                    "Empty publication lost its world identity or retained old vertices.");
            ++empty_records;
            batch.AddVertex(record.vertices.data(), record.vertices.size());
        });
        Require(empty_records == 9 && batch.VertexCount() == 0, "Empty records were skipped or left stale batch vertices.");
        batch.Draw();
        Require(calls.draws == draws_before && calls.uploads == uploads_before,
                "Empty world replacement reissued the previous draw or upload.");
    }
}

int main(int argc, char** argv)
{
    InstallGLStubs();
    try {
        Require(argc == 2, "Expected real block configuration path.");
        CheckCapacityAndStride();
        CheckPartialInitialization();
        CheckEmptyWorldPublication(argv[1]);
        std::cout << "Batch safety and real-world empty-publication handoff passed with GL call stubs, not a GPU context.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
