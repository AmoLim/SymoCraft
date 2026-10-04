#include "renderer/gpu_timer.h"
#include <stdexcept>

namespace SymoCraft {
    GpuTimer::GpuTimer()
    {
        GLint bits = 0;
        glGetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &bits);
        supported_ = bits > 0;
        if (supported_) for (auto& slot : slots_) glGenQueries(2, slot.queries.data());
    }
    GpuTimer::~GpuTimer()
    {
        if (active_draw_) glEndQuery(GL_TIME_ELAPSED);
        if (supported_) for (auto& slot : slots_) glDeleteQueries(2, slot.queries.data());
    }
    void GpuTimer::BeginFrame(std::size_t frame)
    {
        current_ = nullptr;
        if (!supported_) return;
        for (auto& slot : slots_) if (!slot.pending) {
            slot.frame = frame; slot.count = 0; current_ = &slot; break;
        }
    }
    void GpuTimer::BeginDraw()
    {
        if (!current_) return;
        if (current_->count >= current_->queries.size()) throw std::logic_error("GPU timer draw capacity exceeded");
        glBeginQuery(GL_TIME_ELAPSED, current_->queries[current_->count++]);
        active_draw_ = true;
    }
    void GpuTimer::EndDraw()
    {
        if (active_draw_) { glEndQuery(GL_TIME_ELAPSED); active_draw_ = false; }
    }
    void GpuTimer::EndFrame()
    {
        if (current_) current_->pending = true;
        current_ = nullptr;
    }
    std::vector<GpuTimer::Result> GpuTimer::Poll()
    {
        std::vector<Result> results;
        for (auto& slot : slots_) {
            if (!slot.pending) continue;
            bool ready = true;
            for (unsigned i = 0; i < slot.count; ++i) {
                GLint available = GL_FALSE;
                glGetQueryObjectiv(slot.queries[i], GL_QUERY_RESULT_AVAILABLE, &available);
                ready &= available == GL_TRUE;
            }
            if (!ready) continue;
            GLuint64 total = 0;
            for (unsigned i = 0; i < slot.count; ++i) {
                GLuint64 elapsed = 0;
                glGetQueryObjectui64v(slot.queries[i], GL_QUERY_RESULT, &elapsed);
                total += elapsed;
            }
            results.push_back({slot.frame, static_cast<double>(total) / 1000000.0});
            slot.pending = false;
        }
        return results;
    }
}
