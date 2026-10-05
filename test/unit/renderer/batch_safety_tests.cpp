#include "batch.hpp"
#include <array>
#include <cstring>
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
}

int main()
{
    InstallGLStubs();
    try {
        CheckCapacityAndStride();
        CheckPartialInitialization();
        std::cout << "Batch safety tests passed without an OpenGL context.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
