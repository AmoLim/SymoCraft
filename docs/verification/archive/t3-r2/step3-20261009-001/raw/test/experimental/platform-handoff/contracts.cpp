#include <symocraft/platform/window.h>
#include <type_traits>
#include <utility>

#if defined(SDL_MAJOR_VERSION) || defined(_WINDOWS_) || defined(VK_VERSION_1_0) || defined(GL_VERSION_1_0)
#error The public Window consumer must not import a graphics or platform SDK.
#endif

using SymoCraft::CursorMode;
using SymoCraft::InputSnapshot;
using SymoCraft::PointerEvent;
using SymoCraft::Window;
using SymoCraft::WindowMode;

static_assert(std::is_same_v<decltype(&Window::Create), Window* (*)(const char*, int, int, bool, bool, WindowMode)>);
static_assert(std::is_same_v<decltype(Window::Create("declaration-only")), Window*>);
static_assert(!std::is_copy_constructible_v<Window> && !std::is_copy_assignable_v<Window>);
static_assert(!std::is_move_constructible_v<Window> && !std::is_move_assignable_v<Window>);
static_assert(std::is_nothrow_destructible_v<Window>);
static_assert(std::is_same_v<decltype(std::declval<const Window&>().Input()), const InputSnapshot&>);
static_assert(std::is_same_v<decltype(&Window::SetSize), void (Window::*)(int, int)>);
static_assert(std::is_same_v<decltype(&Window::SetCursorMode), void (Window::*)(CursorMode)>);
static_assert(std::is_same_v<decltype(&Window::WaitEvents), void (Window::*)(double)>);
static_assert(std::is_same_v<decltype(&Window::Time), double (*)()>);
static_assert(std::is_same_v<decltype(PointerEvent::mouse_dx), double> &&
              std::is_same_v<decltype(PointerEvent::mouse_dy), double> &&
              std::is_same_v<decltype(PointerEvent::scroll_y), double>);
static_assert(std::is_copy_constructible_v<InputSnapshot>);

#if defined(HANDOFF_GL)
#include <graphics_bridge.h>
using Bridge = SymoCraft::Platform::GraphicsBridge;
static_assert(std::is_same_v<decltype(&Bridge::MakeCurrent), void (*)(Window&)>);
static_assert(std::is_same_v<decltype(&Bridge::HasCurrentContext), bool (*)()>);
static_assert(std::is_same_v<decltype(&Bridge::GetProcedure), Bridge::Procedure (*)(const char*)>);
static_assert(std::is_same_v<decltype(&Bridge::ExtensionSupported), bool (*)(const char*)>);
static_assert(std::is_same_v<decltype(&Bridge::SetVsync), void (*)(bool)>);
static_assert(std::is_same_v<decltype(&Bridge::Present), void (*)(Window&)>);
#elif defined(HANDOFF_NATIVE)
#include <native_bridge.h>
static_assert(std::is_same_v<decltype(&SymoCraft::Platform::NativeBridge::GetHandle), HWND (*)(Window&)>);
#elif defined(HANDOFF_VULKAN)
#include <vulkan_bridge.h>
using Bridge = SymoCraft::Platform::VulkanBridge;
static_assert(std::is_same_v<decltype(&Bridge::GetInstanceProcedureAddress), PFN_vkGetInstanceProcAddr (*)(Window&)>);
static_assert(std::is_same_v<decltype(&Bridge::GetRequiredExtensions), std::vector<std::string> (*)(Window&)>);
static_assert(std::is_same_v<decltype(&Bridge::CreateSurface),
              VkSurfaceKHR (*)(Window&, VkInstance, const VkAllocationCallbacks*)>);
static_assert(std::is_same_v<decltype(&Bridge::DestroySurface),
              void (*)(Window&, VkInstance, VkSurfaceKHR, const VkAllocationCallbacks*)>);
#endif

#include <iostream>

int main() {
    const InputSnapshot snapshot;
    for (bool down : snapshot.keys)
        if (down) return 1;
    if (snapshot.left_button || snapshot.right_button || snapshot.focus_lost || !snapshot.pointer_events.empty()) return 1;
    const PointerEvent pointer;
    if (pointer.mouse_dx != 0 || pointer.mouse_dy != 0 || pointer.scroll_y != 0) return 1;
    std::cout << "Platform handoff declarations and default values passed; no SDL, window or GPU execution.\n";
    return 0;
}
