#pragma once
#include "core.h"
#include "world/world.h"
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <chrono>
#include "renderer/render_stats.h"
#include "renderer/gpu_timer.h"

namespace SymoCraft{

    // Batch Procedure:
    // I.Initialization
    //  1. Create and bind buffers (full DSA)
    //  2. Allocate maximum batch memory to the VBO
    //  3. Configure vertex attributes
    // II.Feed 'data' with vertices
    //   1. Check errors
    //   2. Feed 'data'
    //   3. Tick index
    // III.Draw all vertices in the memory
    //   1. Deliver data to VBO memory
    //   2. Draw vertices
    //   3. Clear the batch

    struct BlockVertex3D{
        glm::ivec3 pos_coord;
        glm::vec3 tex_coord;
        float normal;
    };

    struct LineVertex3D{
        glm::vec3 pos_coord;
    };

    struct VertexAttribute{
        uint16 attribute_slot;
        uint16 element_amount;
        GLenum data_type;
        uint16 offset;
    };

    struct DrawArraysIndirectCommand
    {
        uint32 count{};
        uint32 instanceCount{};
        uint32 first{};
        uint32 baseInstance{};
    };

    template<typename T>
    class Batch
    {
        static_assert(std::is_trivially_copyable_v<T>);
    public:
        Batch() = default;
        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;
        Batch(Batch&&) = delete;
        Batch& operator=(Batch&&) = delete;
        ~Batch() { Free(); }

        static constexpr bool CanAppend(std::size_t count, std::size_t amount, std::size_t capacity) noexcept
        {
            return count <= capacity && amount <= capacity - count;
        }

        void Init(std::initializer_list<VertexAttribute> vertex_attributes)
        {
            ValidateCapacity(m_batch_size);
            Free();
            const auto data_size = static_cast<GLsizeiptr>(sizeof(T) * m_batch_size);
            glCreateBuffers(1, &m_vertex_data_vbo);
            glCreateVertexArrays(1, &m_vao);
            if (!m_vertex_data_vbo || !m_vao)
                throw std::runtime_error("Failed to create batch OpenGL objects.");

            glNamedBufferStorage(m_vertex_data_vbo, data_size, nullptr, GL_DYNAMIC_STORAGE_BIT);
            GLint64 allocated_size = 0;
            glGetNamedBufferParameteri64v(m_vertex_data_vbo, GL_BUFFER_SIZE, &allocated_size);
            if (allocated_size != data_size)
                throw std::runtime_error("Failed to allocate batch vertex buffer storage.");
            glVertexArrayVertexBuffer(m_vao, 0, m_vertex_data_vbo, 0, static_cast<GLsizei>(sizeof(T)));

            for (const auto& attribute : vertex_attributes)
            {
                glEnableVertexArrayAttrib(m_vao, attribute.attribute_slot);
                glVertexArrayAttribFormat(m_vao, attribute.attribute_slot, attribute.element_amount, attribute.data_type, GL_FALSE, attribute.offset);
                glVertexArrayAttribBinding(m_vao, attribute.attribute_slot, 0);
            }
            m_initialized = true;
        }

        void AddVertex(const T& vertex)
        {
            RequireRoom(1);
            data.push_back(vertex);
            m_dirty = true;
        }

        void AddVertex(const T* vertex, std::size_t vertex_amount)
        {
            RequireRoom(vertex_amount);
            if (vertex_amount == 0)
                return;
            if (!vertex)
                throw std::invalid_argument("Cannot append a null vertex range.");
            data.insert(data.end(), vertex, vertex + vertex_amount);
            m_dirty = true;
        }

        void Draw(RenderStats* stats = nullptr, GpuTimer* timer = nullptr)
        {
            RequireInitialized();
            if (data.empty())
                return;
            ReloadData(stats);
            glBindVertexArray(m_vao);
            if (timer) timer->BeginDraw();
            glDrawArrays(m_primitive_type, 0, static_cast<GLsizei>(data.size()));
            if (timer) timer->EndDraw();
            if (stats) { stats->vertices += data.size(); ++stats->draw_calls; }
            glBindVertexArray(0);

            Clear();
        }

        inline void ReloadData(RenderStats* stats = nullptr)
        {
            RequireInitialized();
            if (m_dirty && !data.empty()) {
                const auto start = stats ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
                glNamedBufferSubData(m_vertex_data_vbo, 0,
                    static_cast<GLsizeiptr>(data.size() * sizeof(T)), data.data());
                if (stats) {
                    stats->upload_bytes += data.size() * sizeof(T);
                    stats->upload_cpu_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                }
            }
            m_dirty = false;
        }

        inline void Clear()
        {
            data.clear();
            m_dirty = false;
        }

        inline void Free() noexcept
        {
            if (m_vertex_data_vbo)
                glDeleteBuffers(1, &m_vertex_data_vbo);
            if (m_vao)
                glDeleteVertexArrays(1, &m_vao);
            m_vertex_data_vbo = 0;
            m_vao = 0;
            std::vector<T>().swap(data);
            m_initialized = false;
            m_dirty = false;
        }

        inline void SetPrimitiveType(const GLenum primitive_type)
        {
            m_primitive_type = primitive_type;
        }

        inline void SetBatchSize(const std::size_t new_batch_size)
        {
            if (m_vertex_data_vbo || m_vao)
                throw std::logic_error("Free the batch before changing its capacity.");
            ValidateCapacity(new_batch_size);
            m_batch_size = new_batch_size;
        }

        std::size_t VertexCount() const noexcept { return data.size(); }
        std::size_t AllocatedBytes() const noexcept { return m_initialized ? m_batch_size * sizeof(T) : 0; }

    private:
        uint32 m_vao{};
        uint32 m_vertex_data_vbo{};
        std::size_t m_batch_size{10000000};
        GLenum m_primitive_type{GL_TRIANGLES};
        std::vector<T> data;
        bool m_initialized{};
        bool m_dirty{};

        static void ValidateCapacity(std::size_t capacity)
        {
            if (capacity == 0 || capacity > static_cast<std::size_t>((std::numeric_limits<GLsizei>::max)()) ||
                capacity > static_cast<std::size_t>((std::numeric_limits<GLsizeiptr>::max)()) / sizeof(T))
                throw std::length_error("Batch capacity exceeds OpenGL count or buffer size limits.");
        }

        void RequireInitialized() const
        {
            if (!m_initialized)
                throw std::logic_error("Batch is not initialized.");
        }

        void RequireRoom(std::size_t amount) const
        {
            RequireInitialized();
            if (!CanAppend(data.size(), amount, m_batch_size))
                throw std::length_error("Batch vertex capacity exceeded before append.");
        }
    };

}
