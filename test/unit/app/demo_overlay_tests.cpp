#include "demo_overlay.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace SymoCraft::Application::Detail;
namespace {
void Require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
}
int main() {
    try {
        for (char ch : std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-/%.(),[]+_=|?")) {
            const auto glyph=DemoGlyph(ch);
            Require(!glyph.vertices.empty() && glyph.vertices.size()%3==0,"visible ASCII glyph missing triangles");
            for (const auto& vertex : glyph.vertices) {
                Require(vertex.position.x>=0 && vertex.position.x<=5 && vertex.position.y>=0 && vertex.position.y<=7,
                    "glyph exceeds immutable cell bounds");
                Require(std::abs(glm::length(vertex.normal)-1)<1e-5,"glyph normal invalid");
            }
        }
        Require(DemoGlyph(' ').vertices.empty(),"space must stay empty");
        DemoOverlayData data{"HARVEST DEMO","PAUSED",{{"HUMAN 1 HARVESTING",.5f},{"WOOD=0",{}}}};
        ValidateDemoOverlay(data);
        for (float invalid : {-1.f,1.1f,std::numeric_limits<float>::quiet_NaN()}) {
            data.rows[0].progress=invalid; bool rejected=false;
            try { ValidateDemoOverlay(data); } catch (const std::invalid_argument&) { rejected=true; }
            Require(rejected,"invalid progress must not become successful display");
        }
        data.rows[0].progress.reset(); data.rows[0].label.resize(257,'A');
        bool rejected=false;
        try { ValidateDemoOverlay(data); } catch (const std::length_error&) { rejected=true; }
        Require(rejected,"unbounded row must be rejected before GPU allocation");
        std::cout<<"demo overlay contracts passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
