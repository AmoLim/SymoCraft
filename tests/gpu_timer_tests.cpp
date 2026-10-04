#include "renderer/gpu_timer.h"
#include <iostream>
#include <stdexcept>

namespace {
    GLuint next_id = 1;
    unsigned begins = 0, reads = 0, deletes = 0;
    bool ready = false;
    void APIENTRY GetTarget(GLenum, GLenum, GLint* value) { *value = 64; }
    void APIENTRY Generate(GLsizei count, GLuint* ids) { while (count--) *ids++ = next_id++; }
    void APIENTRY Delete(GLsizei count, const GLuint*) { deletes += count; }
    void APIENTRY Begin(GLenum, GLuint) { ++begins; }
    void APIENTRY End(GLenum) {}
    void APIENTRY Available(GLuint, GLenum, GLint* value) { *value = ready; }
    void APIENTRY Read(GLuint, GLenum, GLuint64* value)
    {
        if (!ready) throw std::runtime_error("Read blocked on an unavailable GPU result");
        ++reads; *value = 1000000;
    }
    void Require(bool condition) { if (!condition) throw std::runtime_error("GPU query ring contract failed"); }
}
int main()
{
    try {
        glad_glGetQueryiv = GetTarget; glad_glGenQueries = Generate; glad_glDeleteQueries = Delete;
        glad_glBeginQuery = Begin; glad_glEndQuery = End; glad_glGetQueryObjectiv = Available;
        glad_glGetQueryObjectui64v = Read;
        {
            SymoCraft::GpuTimer timer;
            for (std::size_t frame = 0; frame < 65; ++frame) {
                timer.BeginFrame(frame);
                timer.BeginDraw(); timer.EndDraw(); timer.BeginDraw(); timer.EndDraw(); timer.EndFrame();
            }
            Require(begins == 128 && timer.Poll().empty() && reads == 0);
            ready = true;
            const auto results = timer.Poll();
            Require(results.size() == 64 && results.front().frame == 0 && results.back().frame == 63 && results.front().milliseconds == 2);
            timer.BeginFrame(65); timer.BeginDraw(); timer.EndDraw(); timer.EndFrame();
            const auto reused = timer.Poll();
            Require(reused.size() == 1 && reused.front().frame == 65 && reused.front().milliseconds == 1);
        }
        Require(deletes == 128);
        std::cout << "GPU query readiness, saturation, frame IDs and cleanup passed without a context.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
