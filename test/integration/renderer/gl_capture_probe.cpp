#include <symocraft/platform/window.h>
#include <symocraft/renderer/legacy_renderer.h>
#include <symocraft/assets/image.h>
#include <glad/glad.h>
#include <iostream>
#include <stdexcept>

// Explicit native GL readback regression; never an unattended CTest window.
int main(int argc, char** argv) {
    if (argc != 2 || std::filesystem::exists(argv[1])) return 64;
    try {
        SymoCraft::Platform::Runtime runtime;
        auto window = SymoCraft::Window::Create(runtime, "Symocraft GL capture probe", 320, 240, false, true);
        SymoCraft::Renderer::AttachContext(*window);
        glClearColor(0, 0, 1, 1); glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, window->Width(), window->Height() / 2);
        glClearColor(1, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        SymoCraft::Renderer::CaptureFramebuffer(window->Width(), window->Height(), argv[1]);
        const auto image = SymoCraft::Assets::DecodeImage(argv[1]);
        if (image.width != window->Width() || image.height != window->Height() || image.channels != 3)
            throw std::runtime_error("GL screenshot extent/channel ownership changed");
        const auto pixel = [&](int y, unsigned channel) { return image.pixels[(std::size_t(y) * image.width + image.width / 2) * 3 + channel]; };
        if (pixel(image.height / 4, 2) < 250 || pixel(image.height / 4, 0) > 5 ||
            pixel(image.height * 3 / 4, 0) < 250 || pixel(image.height * 3 / 4, 2) > 5)
            throw std::runtime_error("GL screenshot readback or top-left orientation changed");
        window->Destroy(); window.reset(); runtime.Shutdown();
        std::cout << "GL native screenshot pixels, orientation and cleanup passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
