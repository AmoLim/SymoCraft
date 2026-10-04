#include "core/performance.h"
#include "core/startup_options.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <numeric>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace SymoCraft::Performance {
    namespace {
        std::ofstream Output(const std::filesystem::path& path)
        {
            std::ofstream file(path);
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.imbue(std::locale::classic());
            file << std::setprecision(10);
            return file;
        }
        template<class T> void Optional(std::ostream& out, const std::optional<T>& value)
        { if (value) out << *value; }
    }
    YAML::Node Statistics(std::vector<double> values)
    {
        YAML::Node node;
        node["count"] = values.size();
        if (values.empty()) return node;
        for (const auto value : values)
            if (!std::isfinite(value) || value < 0) throw std::invalid_argument("Invalid performance sample");
        std::sort(values.begin(), values.end());
        node["min"] = values.front();
        node["max"] = values.back();
        node["mean"] = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        for (const auto& [name, percentile] : {std::pair{"p50", .5}, {"p95", .95}, {"p99", .99}})
            node[name] = values[static_cast<std::size_t>(std::ceil(percentile * values.size())) - 1];
        return node;
    }
    Session::Session(const StartupOptions& options, Clock::time_point entry_time)
        : entry(entry_time), directory_(std::u8string(options.output_directory.begin(), options.output_directory.end())),
          allow_unfocused_(options.focus_policy == "allow-unfocused")
    {
        if (directory_.empty() || !std::filesystem::create_directory(directory_))
            throw std::runtime_error("Benchmark output must be a new directory with an existing parent");
        // Bound collection memory; exporting and sorting happen after the measured loop.
        frames_.reserve(100000);
        memory_.reserve(1300);
        metadata["schema_version"] = 2;
        metadata["protocol_version"] = 2;
        metadata["workload_version"] = 2;
        metadata["focus_policy"] = std::string(options.focus_policy);
        metadata["benchmark"] = std::string(options.benchmark);
        metadata["warmup_seconds"] = options.warmup_seconds;
        metadata["sample_seconds"] = options.sample_seconds;
        metadata["requested_framebuffer_width"] = options.width;
        metadata["requested_framebuffer_height"] = options.height;
        metadata["requested_vsync"] = options.vsync;
        metadata["percentile_method"] = "nearest-rank, ceil(p*N)-1; no outlier removal";
        metadata["frame_time_scope"] = "consecutive SwapBuffers return intervals; not physical display latency";
        metadata["gpu_time_scope"] = "sum of GL_TIME_ELAPSED around draw calls, excluding explicit uploads/clear/swap";
        metadata["memory_scope"] = "process working set/private bytes; NVX memory is device-wide driver estimate, not process VRAM";
        metadata["cpu_temperature"] = "missing; no portable sensor interface enabled";
        metadata["gpu_temperature"] = "not collected by game; an external collector may supply it separately";
#ifdef NDEBUG
        metadata["build_configuration"] = "Release";
#else
        metadata["build_configuration"] = "Debug";
#endif
        metadata["msvc_version"] = _MSC_FULL_VER;
        Status("initializing");
    }
    void Session::Status(const char* phase, double elapsed)
    {
        YAML::Node state;
        state["protocol_version"] = 2;
        state["phase"] = phase;
        state["elapsed_seconds"] = elapsed;
        const auto temporary = directory_ / "status.tmp";
        auto file = Output(temporary);
        file << YAML::Dump(state) << '\n';
        file.close();
        if (!MoveFileExW(temporary.c_str(), (directory_ / "status.yaml").c_str(), MOVEFILE_REPLACE_EXISTING))
            throw std::runtime_error("Cannot publish benchmark status");
    }
    void Session::Add(Frame frame)
    {
        if (frames_.size() == 1000000) throw std::runtime_error("Benchmark frame capacity exceeded; partial data retained");
        if (!frame.focused && !allow_unfocused_) Invalidate("window-not-focused");
        frames_.push_back(frame);
    }
    void Session::SetGpu(std::size_t frame, double milliseconds)
    {
        if (frame >= frames_.size()) throw std::logic_error("GPU result has no matching CPU frame");
        frames_[frame].gpu_draw_ms = milliseconds;
    }
    void Session::Invalidate(const std::string& reason)
    {
        if (std::find(invalid_reasons_.begin(), invalid_reasons_.end(), reason) == invalid_reasons_.end())
            invalid_reasons_.push_back(reason);
    }
    bool Session::Export(bool normal_exit)
    {
        Status("exporting");
        if (!normal_exit) Invalidate("runtime-error");
        if (!completed) Invalidate("duration-not-completed");
        auto csv = Output(directory_ / "frames.csv");
        csv << "frame,phase,elapsed_s,frame_ms,simulation_delta_ms,event_ms,simulation_ms,mesh_ms,pack_ms,render_cpu_ms,upload_cpu_ms,present_ms,instrumentation_ms,gpu_draw_ms,rebuilt_chunks,vertices,upload_bytes,draw_calls,edits,edit_lateness_ms,x,y,z,yaw,focused\n";
        std::vector<double> frame_times, gpu_times, mesh_times, upload_times, submit_times, present_times;
        std::uint64_t edits = 0, rebuilt = 0;
        std::uint64_t unfocused_frames = 0, sample_unfocused_frames = 0, transitions = 0;
        double unfocused_ms = 0, sample_unfocused_ms = 0;
        auto focus = Output(directory_ / "focus.csv");
        focus << "frame,elapsed_s,focused\n";
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            const auto& f = frames_[i];
            if (i == 0 || frames_[i - 1].focused != f.focused) {
                focus << i << ',' << f.elapsed << ',' << f.focused << '\n';
                if (i != 0) ++transitions;
            }
            if (!f.focused) {
                ++unfocused_frames; unfocused_ms += f.frame_ms;
                if (f.measured) { ++sample_unfocused_frames; sample_unfocused_ms += f.frame_ms; }
            }
            csv << i << ',' << (f.measured ? "sample" : "warmup") << ',' << f.elapsed << ',' << f.frame_ms << ','
                << f.simulation_delta_ms << ',' << f.event_ms << ',' << f.simulation_ms << ',' << f.mesh_ms << ','
                << f.pack_ms << ',' << f.render_cpu_ms << ',' << f.upload_cpu_ms << ',' << f.present_ms << ',' << f.instrumentation_ms << ',';
            Optional(csv, f.gpu_draw_ms);
            csv << ',' << f.rebuilt_chunks << ',' << f.vertices << ',' << f.upload_bytes << ',' << f.draw_calls << ','
                << f.edits << ',' << f.edit_lateness_ms << ',' << f.x << ',' << f.y << ',' << f.z << ',' << f.yaw << ',' << f.focused << '\n';
            if (!f.measured) continue;
            frame_times.push_back(f.frame_ms); mesh_times.push_back(f.mesh_ms);
            upload_times.push_back(f.upload_cpu_ms); present_times.push_back(f.present_ms);
            submit_times.push_back(f.pack_ms + f.render_cpu_ms - f.upload_cpu_ms);
            if (f.gpu_draw_ms) gpu_times.push_back(*f.gpu_draw_ms);
            edits += f.edits; rebuilt += f.rebuilt_chunks;
        }
        csv.close();
        focus.close();
        auto memory = Output(directory_ / "memory.csv");
        memory << "elapsed_s,working_set_bytes,private_bytes,device_dedicated_kib,device_available_kib\n";
        for (const auto& m : memory_) {
            memory << m.elapsed << ','; Optional(memory, m.working_set_bytes); memory << ',';
            Optional(memory, m.private_bytes); memory << ','; Optional(memory, m.device_dedicated_kib); memory << ',';
            Optional(memory, m.device_available_kib); memory << '\n';
        }
        memory.close();
        YAML::Node report;
        report["metadata"] = metadata;
        report["startup_ms"] = startup;
        report["completed"] = completed;
        report["valid_run"] = normal_exit && completed && invalid_reasons_.empty() && !frame_times.empty();
        report["invalid_reasons"] = invalid_reasons_;
        report["total_frames"] = frames_.size();
        report["focus"]["transitions"] = transitions;
        report["focus"]["unfocused_frames"] = unfocused_frames;
        report["focus"]["sample_unfocused_frames"] = sample_unfocused_frames;
        report["focus"]["unfocused_seconds_estimate"] = unfocused_ms / 1000;
        report["focus"]["sample_unfocused_seconds_estimate"] = sample_unfocused_ms / 1000;
        report["focus"]["duration_method"] = "sum frame intervals classified by focus at event polling; approximate";
        report["frame_ms"] = Statistics(frame_times);
        report["gpu_draw_ms"] = Statistics(gpu_times);
        report["gpu_missing_sample_frames"] = frame_times.size() - gpu_times.size();
        report["mesh_ms"] = Statistics(mesh_times);
        report["upload_cpu_ms"] = Statistics(upload_times);
        report["cpu_submit_excluding_upload_ms"] = Statistics(submit_times);
        report["present_wait_cpu_ms"] = Statistics(present_times);
        report["sample_edits"] = edits;
        report["sample_rebuilt_chunks"] = rebuilt;
        const double duration = std::accumulate(frame_times.begin(), frame_times.end(), 0.0);
        if (duration > 0) report["throughput_fps"] = frame_times.size() * 1000.0 / duration;
        auto yaml = Output(directory_ / "summary.yaml");
        yaml << YAML::Dump(report) << '\n';
        yaml.close();
        Status("finished");
        return report["valid_run"].as<bool>();
    }
}
