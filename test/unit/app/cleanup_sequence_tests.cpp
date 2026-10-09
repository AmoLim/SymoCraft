#include "cleanup_sequence.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    using SymoCraft::Application::Detail::CleanupSequence;

    void Require(bool value) {
        if (!value) throw std::runtime_error("Application cleanup contract failed");
    }

    void CheckOrderAndRepeatedCleanup() {
        bool renderer_active = true;
        bool video_active = true;
        auto window = std::make_unique<int>(1);
        std::vector<std::string> order;
        std::ostringstream diagnostics;
        const auto release = [&] {
            CleanupSequence cleanup(diagnostics);
            cleanup.RunOnce("renderer", renderer_active, [&] { order.emplace_back("renderer"); });
            cleanup.Run("context/window", [&] { if (window) order.emplace_back("context/window"); });
            cleanup.Run("window owner", [&] { window.reset(); });
            cleanup.RunOnce("video", video_active, [&] { order.emplace_back("video"); });
            cleanup.RethrowFailure();
        };
        release();
        release();
        Require(order == std::vector<std::string>{"renderer", "context/window", "video"});
        Require(!renderer_active && !video_active && !window && diagnostics.str().empty());
    }

    void CheckSecondaryFailuresAndState() {
        bool renderer_active = true;
        bool video_active = true;
        auto window = std::make_unique<int>(1);
        std::vector<std::string> order;
        std::ostringstream diagnostics;
        CleanupSequence cleanup(diagnostics);
        cleanup.RunOnce("Renderer::Free", renderer_active, [&] {
            order.emplace_back("renderer");
            throw std::runtime_error("first owned diagnostic");
        });
        cleanup.Run("Window::Destroy", [&] {
            order.emplace_back("context/window");
            throw std::runtime_error("second owned diagnostic");
        });
        cleanup.Run("window owner", [&] { window.reset(); });
        cleanup.RunOnce("Window::Free", video_active, [&] { order.emplace_back("video"); });
        bool rejected = false;
        try {
            cleanup.RethrowFailure();
        } catch (const std::runtime_error& error) {
            rejected = true;
            const std::string message = error.what();
            Require(message.find("Renderer::Free: first owned diagnostic") != std::string::npos);
            Require(message.find("second owned diagnostic") == std::string::npos);
        }
        Require(rejected && !renderer_active && !video_active && !window);
        Require(order == std::vector<std::string>{"renderer", "context/window", "video"});
        Require(diagnostics.str().find("Window::Destroy: second owned diagnostic") != std::string::npos);

        CleanupSequence repeated(diagnostics);
        repeated.RunOnce("Renderer::Free", renderer_active, [] { throw std::runtime_error("stale renderer retry"); });
        repeated.RunOnce("Window::Free", video_active, [] { throw std::runtime_error("stale video retry"); });
        repeated.RethrowFailure();
    }

    void CheckNonStandardFailureAndBrokenDiagnosticSink() {
        std::ostringstream diagnostics;
        diagnostics.exceptions(std::ios::badbit);
        try { diagnostics.setstate(std::ios::badbit); } catch (const std::ios_base::failure&) {}
        bool remaining_release = false;
        CleanupSequence cleanup(diagnostics);
        cleanup.Run("first release", [] { throw 7; });
        cleanup.Run("remaining release", [&] { remaining_release = true; });
        bool rejected = false;
        try {
            cleanup.RethrowFailure();
        } catch (const std::runtime_error& error) {
            rejected = true;
            Require(std::string(error.what()).find("first release: unexpected non-standard exception") != std::string::npos);
        }
        Require(remaining_release && rejected);
    }
}

int main() {
    try {
        CheckOrderAndRepeatedCleanup();
        CheckSecondaryFailuresAndState();
        CheckNonStandardFailureAndBrokenDiagnosticSink();
        std::cout << "Ordered application cleanup, failure isolation and repeated-release contracts passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
