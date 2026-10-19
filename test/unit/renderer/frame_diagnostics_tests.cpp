#include "frame_diagnostics.h"
#include <iostream>
#include <numeric>
#include <vector>

using namespace SymoCraft::Rendering::Detail;
namespace {
struct Fake {
    unsigned calls{};
    bool missing{};
    std::vector<FrameDiagnosticRecord> records;
    static FrameDiagnosticStamp Now(void* context) noexcept {
        auto& self = *static_cast<Fake*>(context); ++self.calls;
        return {std::chrono::steady_clock::time_point(std::chrono::nanoseconds(self.calls * 10)),
                self.missing ? std::nullopt : std::optional<std::uint64_t>(self.calls * 4)};
    }
    static void Accept(void* context, const FrameDiagnosticRecord& record) noexcept {
        static_cast<Fake*>(context)->records.push_back(record);
    }
};
void Require(bool ok) { if (!ok) throw std::runtime_error("frame diagnostic contract failed"); }
}
int main() {
    try {
        Fake fake; fake.records.reserve(6);
        FrameDiagnosticSink sink{&fake, Fake::Now, Fake::Accept};
        { FrameDiagnosticScope off(true, 1, false); DiagnosticPhase(FramePhase::CpuPack); }
        Require(fake.calls == 0 && current_frame_diagnostic == nullptr);
        {
            ScopedFrameDiagnostics binding(sink);
            { FrameDiagnosticScope off(false, 2, false); DiagnosticPhase(FramePhase::CpuPack); }
            Require(fake.calls == 0);
            bool rejected = false;
            try { ScopedFrameDiagnostics second(sink); } catch (const std::logic_error&) { rejected = true; }
            Require(rejected);
            {
                FrameDiagnosticScope frame(true, 3, true);
                DiagnosticPhase(FramePhase::CpuPack); DiagnosticPhase(FramePhase::CpuPack);
                frame.Complete();
            }
            Require(fake.calls == 4 && fake.records.size() == 1 && current_frame_diagnostic == nullptr);
            const auto& r = fake.records[0];
            Require(r.frame_id == 3 && r.screenshot && r.completed && r.cpu_valid);
            Require(r.wall_ns[0] == 10 && r.wall_ns[unsigned(FramePhase::CpuPack)] == 20);
            Require(r.visits[unsigned(FramePhase::CpuPack)] == 2);
            Require(std::accumulate(r.wall_ns.begin(), r.wall_ns.end(), std::uint64_t{}) == 30);
            Require(std::accumulate(r.cpu_ns.begin(), r.cpu_ns.end(), std::uint64_t{}) == 12);
            fake.missing = true;
            try { FrameDiagnosticScope failed(true, 4, false); throw std::runtime_error("expected"); } catch (...) {}
            Require(fake.records.size() == 2 && !fake.records[1].completed && !fake.records[1].cpu_valid);
            {
                DiagnosticWorkScope update(FrameWork::MeshUpdateTotal);
                DiagnosticWorkScope copy(FrameWork::MeshOwnedCopy);
            }
            {
                FrameDiagnosticScope frame(true, 5, false);
                DiagnosticWorkScope scan(FrameWork::LayerScan, true, 42);
                frame.Complete();
            }
            const auto& work = fake.records.back().work;
            Require(work.visits[unsigned(FrameWork::MeshUpdateTotal)] == 1);
            Require(work.visits[unsigned(FrameWork::MeshOwnedCopy)] == 1);
            Require(work.visits[unsigned(FrameWork::LayerScan)] == 1 && work.layer_vertices == 42);
            {
                FrameDiagnosticScope frame(true, 6, false);
                DiagnosticWorkScope summary(FrameWork::LayerSummaryCheck);
                frame.Complete();
            }
            Require(fake.records.back().work.visits[unsigned(FrameWork::MeshUpdateTotal)] == 0);
            Require(fake.records.back().work.visits[unsigned(FrameWork::LayerSummaryCheck)] == 1);
            Require(fake.records.back().work.visits[unsigned(FrameWork::LayerScan)] == 0 && fake.records.back().work.layer_vertices == 0);
        }
        Require(frame_diagnostic_sink == nullptr);
        { DiagnosticWorkScope off(FrameWork::MeshUpdateTotal); }
        Require(pending_frame_work.visits[unsigned(FrameWork::MeshUpdateTotal)] == 0);
        std::cout << "frame diagnostic disabled/binding/partition/missing/error contracts passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
