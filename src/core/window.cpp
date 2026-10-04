//
// Created by Amo on 2022/6/15.
//

#include "core/window.h"
#include "core.h"
#include <memory>
#include <stdexcept>

namespace SymoCraft
{
    namespace
    {
        bool glfw_initialized = false;

        void GlfwErrorCallback(int code, const char* description)
        {
            std::fprintf(stderr, "GLFW error %d: %s\n", code, description ? description : "unknown");
        }

        std::runtime_error GlfwFailure(const char* operation)
        {
            const char* description = nullptr;
            glfwGetError(&description);
            return std::runtime_error(std::string(operation) + ": " +
                                      (description ? description : "no additional GLFW detail"));
        }
    }

// User resize window callback func
// Static
    static void ResizeCallback(GLFWwindow* window_ptr, int new_width, int new_height)
    {
        Window* user_window = (Window*) glfwGetWindowUserPointer(window_ptr);
        if (!user_window)
            return;
        user_window->width = new_width;
        user_window->height = new_height;
        glViewport(0, 0, new_width, new_height);
    }

// Static functions
    void Window::Init()
    {
        if (glfw_initialized)
            return;
        glfwSetErrorCallback(GlfwErrorCallback);
        if (glfwInit() != GLFW_TRUE)
            throw GlfwFailure("Failed to initialize GLFW");
        glfw_initialized = true;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);    // Multisample Anti-aliasing
#ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    }

    void Window::Free()
    {
        if (glfw_initialized)
        {
            glfwTerminate();
            glfw_initialized = false;
        }
    }


    Window* Window::Create(const char *window_title, int requested_width, int requested_height)
    {
        auto res = std::make_unique<Window>();

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
            throw GlfwFailure("Failed to get primary monitor");
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
            throw GlfwFailure("Failed to get video mode of primary monitor");
        }
        AmoLogger_Info("Monitor size: %d, %d", mode->width, mode->height);

        // The smallest monitor size accepted is 800 * 600
        res->width = requested_width > 0 ? requested_width : glm::clamp(mode->width / 2, 800, INT_MAX);
        res->height = requested_height > 0 ? requested_height : glm::clamp(mode->height / 2, 600, INT_MAX);
        res->title = window_title;

        res->window_ptr = (void*) glfwCreateWindow(res->width, res->height, window_title, nullptr, nullptr);
        if (res->window_ptr == nullptr)
        {
            throw GlfwFailure("Failed to create an OpenGL 4.6 window");
        }
        try
        {
            AmoLogger_Info("Window created. ");
            glfwSetWindowUserPointer((GLFWwindow*)res->window_ptr, res.get());
            res->MakeContextCurrent();
            if (glfwGetCurrentContext() != res->window_ptr)
                throw GlfwFailure("Failed to make the OpenGL context current");

            int monitor_x, monitor_y;
            glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);
            int window_width, window_height;
            glfwGetWindowSize((GLFWwindow*)res->window_ptr, &window_width, &window_height);
            glfwSetWindowPos((GLFWwindow*)res->window_ptr,
                             monitor_x + (mode->width - window_width) / 2,
                             monitor_y + (mode->height - window_height) / 2);
            res->SetVsync(true);

            if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
                throw std::runtime_error("Failed to load OpenGL functions through GLAD");
            if (!GLAD_GL_VERSION_4_6)
                throw std::runtime_error("OpenGL 4.6 is required by the shaders and renderer");

            glfwGetFramebufferSize((GLFWwindow*)res->window_ptr, &res->width, &res->height);
            glfwSetFramebufferSizeCallback((GLFWwindow*)res->window_ptr, ResizeCallback);
            glViewport(0, 0, res->width, res->height);

            const auto vendor = glGetString(GL_VENDOR);
            const auto renderer = glGetString(GL_RENDERER);
            const auto version = glGetString(GL_VERSION);
            if (!vendor || !renderer || !version)
                throw std::runtime_error("Could not query the current OpenGL device");
            std::cout << "OpenGL vendor: " << vendor << '\n'
                      << "OpenGL renderer: " << renderer << '\n'
                      << "OpenGL version: " << version << std::endl;
        }
        catch (...)
        {
            res->Destroy();
            throw;
        }

        return res.release();
    }


    void Window::MakeContextCurrent()
    {
        glfwMakeContextCurrent((GLFWwindow*)window_ptr);
    }

    void Window::PollInt()
    {
        //
        glfwPollEvents();
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers((GLFWwindow*)window_ptr);
    }

    bool Window::ShouldClose()
    {
        return !window_ptr || glfwWindowShouldClose((GLFWwindow*)window_ptr);
    }





    void Window::Close()
    {
        if (window_ptr)
            glfwSetWindowShouldClose((GLFWwindow*)window_ptr, true);
    }

    void Window::Destroy()
    {
        if (window_ptr)
        {
            glfwDestroyWindow((GLFWwindow*)window_ptr);
            window_ptr = nullptr;
        }
    }

    void Window::SetCursorMode(CursorMode cursorMode)
    {
        int glfw_cursor_mode = GLFW_CURSOR_NORMAL;
        switch (cursorMode)
        {
            case CursorMode::Lock:
                glfw_cursor_mode = GLFW_CURSOR_DISABLED;
                break;
            case CursorMode::Normal:
                glfw_cursor_mode = GLFW_CURSOR_NORMAL;
                break;
            case CursorMode::Hidden:
                glfw_cursor_mode = GLFW_CURSOR_HIDDEN;
                break;
            default:
                break;
        }
        glfwSetInputMode((GLFWwindow*)window_ptr, GLFW_CURSOR, glfw_cursor_mode);
    }

    void Window::SetVsync(bool on)
    {
        if (on)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
    }

    void Window::SetTitle(const char *new_title)
    {
        glfwSetWindowTitle((GLFWwindow*)window_ptr, new_title);
    }

    void Window::SetSize(int width, int height)
    {
        glfwSetWindowSize((GLFWwindow*)window_ptr, width, height);
    }

    float Window::GetAspectRatio() const
    {
        return static_cast<float>(std::max(width, 1)) / static_cast<float>(std::max(height, 1));
    }


}
