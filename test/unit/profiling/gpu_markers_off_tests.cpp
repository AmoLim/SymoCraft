#include "d3d12/debug_markers.h"
#include "vulkan/debug_markers.h"

int main() {
    // Undefined arguments also prove that neither native API nor formatting is needed.
    SYMOCRAFT_D3D12_NAME(undefined_object, undefined_name());
    SYMOCRAFT_D3D12_REGION(undefined_command(), undefined_label());
    SYMOCRAFT_D3D12_REGION_IF(undefined_command(), undefined_label(), undefined_condition());
    SYMOCRAFT_VK_NAME(undefined_native, undefined_object, undefined_type, undefined_label());
    SYMOCRAFT_VK_REGION(undefined_native, undefined_command(), undefined_label());
    SYMOCRAFT_VK_REGION_IF(undefined_native, undefined_command(), undefined_label(), undefined_condition());
}
