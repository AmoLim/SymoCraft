#include "demo_overlay.h"
#include <array>
#include <cmath>
#include <cctype>
#include <stdexcept>

namespace SymoCraft::Application::Detail {
namespace {
void Rectangle(CharacterMeshData& mesh, float x, float y, float w, float h, glm::vec3 color) {
    const auto normal = glm::normalize(glm::vec3(.4f, 1.f, .7f));
    const glm::vec3 p[]{ {x,y,0}, {x+w,y,0}, {x+w,y+h,0}, {x,y+h,0} };
    // Both faces permit the same screen-space glyph input on both viewport conventions.
    for (int i : {0,1,2,0,2,3,2,1,0,3,2,0}) mesh.vertices.push_back({p[i],normal,color});
}
std::array<unsigned char,7> Pattern(char value) {
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(value)))) {
    case 'A': return {14,17,17,31,17,17,17}; case 'B': return {30,17,17,30,17,17,30};
    case 'C': return {14,17,16,16,16,17,14}; case 'D': return {30,17,17,17,17,17,30};
    case 'E': return {31,16,16,30,16,16,31}; case 'F': return {31,16,16,30,16,16,16};
    case 'G': return {14,17,16,23,17,17,15}; case 'H': return {17,17,17,31,17,17,17};
    case 'I': return {14,4,4,4,4,4,14}; case 'J': return {7,2,2,2,18,18,12};
    case 'K': return {17,18,20,24,20,18,17}; case 'L': return {16,16,16,16,16,16,31};
    case 'M': return {17,27,21,21,17,17,17}; case 'N': return {17,25,21,19,17,17,17};
    case 'O': return {14,17,17,17,17,17,14}; case 'P': return {30,17,17,30,16,16,16};
    case 'Q': return {14,17,17,17,21,18,13}; case 'R': return {30,17,17,30,20,18,17};
    case 'S': return {15,16,16,14,1,1,30}; case 'T': return {31,4,4,4,4,4,4};
    case 'U': return {17,17,17,17,17,17,14}; case 'V': return {17,17,17,17,17,10,4};
    case 'W': return {17,17,17,21,21,21,10}; case 'X': return {17,17,10,4,10,17,17};
    case 'Y': return {17,17,10,4,4,4,4}; case 'Z': return {31,1,2,4,8,16,31};
    case '0': return {14,17,19,21,25,17,14}; case '1': return {4,12,4,4,4,4,14};
    case '2': return {14,17,1,2,4,8,31}; case '3': return {30,1,1,14,1,1,30};
    case '4': return {2,6,10,18,31,2,2}; case '5': return {31,16,16,30,1,1,30};
    case '6': return {14,16,16,30,17,17,14}; case '7': return {31,1,2,4,8,8,8};
    case '8': return {14,17,17,14,17,17,14}; case '9': return {14,17,17,15,1,1,14};
    case ':': return {0,4,4,0,4,4,0}; case '.': return {0,0,0,0,0,4,4};
    case ',': return {0,0,0,0,4,4,8}; case '-': return {0,0,0,31,0,0,0};
    case '/': return {1,2,2,4,8,8,16}; case '%': return {17,2,4,4,8,16,17};
    case '[': return {14,8,8,8,8,8,14}; case ']': return {14,2,2,2,2,2,14};
    case '(': return {2,4,8,8,8,4,2}; case ')': return {8,4,2,2,2,4,8};
    case '+': return {0,4,4,31,4,4,0}; case '=': return {0,0,31,0,31,0,0};
    case '_': return {0,0,0,0,0,0,31}; case '|': return {4,4,4,4,4,4,4};
    case ' ': return {};
    default: return {14,17,1,2,4,0,4};
    }
}
}
CharacterMeshData DemoGlyph(char character) {
    CharacterMeshData mesh;
    const auto pattern = Pattern(character);
    for (int y=0;y<7;++y) for (int x=0;x<5;++x)
        if (pattern[y] & (1u << (4-x))) Rectangle(mesh, static_cast<float>(x), static_cast<float>(y), 1, 1, {1,1,1});
    return mesh;
}
CharacterMeshData DemoRectangle(glm::vec3 color) {
    CharacterMeshData mesh; Rectangle(mesh,0,0,1,1,color); return mesh;
}
void ValidateDemoOverlay(const DemoOverlayData& data) {
    if (data.title.size()>100 || data.state.size()>100 || data.rows.size()>12)
        throw std::length_error("demo overlay exceeds display limits");
    for (const auto& row : data.rows) {
        if (row.label.size()>256) throw std::length_error("demo overlay row exceeds display limits");
        if (row.progress && (!std::isfinite(*row.progress) || *row.progress<0 || *row.progress>1))
            throw std::invalid_argument("demo progress must be finite and normalized");
    }
}
}
