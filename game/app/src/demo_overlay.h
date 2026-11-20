#pragma once

#include <symocraft/scene/character_mesh.h>
#include <optional>
#include <string>
#include <vector>

namespace SymoCraft::Application::Detail {
struct DemoOverlayRow {
    std::string label;
    std::optional<float> progress;
};
struct DemoOverlayData {
    std::string title;
    std::string state;
    std::vector<DemoOverlayRow> rows;
};
// A tiny built-in bitmap alphabet keeps this demo independent of font libraries
// and external files. The geometry is immutable and shared across all frames.
CharacterMeshData DemoGlyph(char character);
CharacterMeshData DemoRectangle(glm::vec3 color);
void ValidateDemoOverlay(const DemoOverlayData& data);
}
