# T2 S4 platform declaration consumers

This isolated CMake 3.22 project compiles the actual public Window and private
bridge declarations used by the [S4 handoff](../../../docs/milestones/m3-t2/s4-platform-handoff.md).
It does not configure SDL, compile Window, link production archives, initialize
video or create GPU objects. Runtime checks only inspect CPU default values;
declaration compatibility is enforced at compile time.

The public consumer sees only the platform public include directory. The GL
consumer adds only the private bridge directory and needs no GLAD/SDL headers.
The Native consumer adds Windows SDK declarations; Vulkan is opt-in and uses an
explicit SDK Include directory. No target links a Vulkan import library.

In an x64 MSVC development terminal, using a fresh build directory:

```powershell
cmake -S test/experimental/platform-handoff -B out/m3-t2/s4/contracts-local-001 `
    -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build out/m3-t2/s4/contracts-local-001
ctest --test-dir out/m3-t2/s4/contracts-local-001 --output-on-failure --no-tests=error
```

For the additional Vulkan consumer, use a separate directory and append
`-DSYMOCRAFT_HANDOFF_VULKAN=ON` and
`-DSYMOCRAFT_VULKAN_HEADERS_ROOT=C:/VulkanSDK/1.4.363.0/Include` to configure.
Match the real installed SDK path. Repeat Debug and Release as needed.

The SDK-free configuration has three tests, and the explicit Vulkan configuration
has four. Passing them proves signatures, owning/borrowed C++ type shapes and
default CPU values, not resource lifetimes, actual DPI, SDK-free deployment under
a missing system loader, modern rendering or final T2/T3 acceptance. Existing
real bridge and game observations retain their own source/archive identities.
