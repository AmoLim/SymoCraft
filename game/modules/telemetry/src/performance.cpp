#include "symocraft/telemetry/performance.h"
#include "symocraft/telemetry/document_io.h"
#include "symocraft/foundation/files.h"
#include "export_diagnostics/export_probe.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>
#include <system_error>

namespace SymoCraft::Performance {
    namespace {
        thread_local Detail::ExportProbe export_probe = nullptr;
        thread_local void* export_probe_context = nullptr;
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
        void ValidateDuration(double milliseconds)
        {
            if (!std::isfinite(milliseconds) || milliseconds < 0)
                throw std::invalid_argument("Invalid performance sample");
        }
        void CsvString(std::ostream& out, const std::string& value)
        {
            out << '"';
            for (const auto character : value) {
                if (character == '"') out << '"';
                out << character;
            }
            out << '"';
        }
        std::uint64_t AddCounter(std::uint64_t value, std::uint64_t increment)
        {
            if (increment > std::numeric_limits<std::uint64_t>::max() - value)
                throw std::overflow_error("Performance counter overflow");
            return value + increment;
        }
    }
    void Detail::SetExportProbe(ExportProbe probe, void* context) noexcept {
        export_probe = probe; export_probe_context = context;
    }
    Data::Value Statistics(std::vector<double> values)
    {
        Data::Value node;
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
    Session::Session(const SessionConfig& options, Clock::time_point entry_time)
        : entry(entry_time), directory_(std::u8string(options.output_directory.begin(), options.output_directory.end())),
          allow_unfocused_(options.focus_policy == "allow-unfocused")
    {
        if (directory_.empty() || !std::filesystem::create_directory(directory_))
            throw std::runtime_error("Benchmark output must be a new directory with an existing parent");
        // Bound collection memory; exporting and sorting happen after the measured loop.
        frames_.reserve(100000);
        gpu_resolved_.reserve(100000);
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
        RequireRecording("Status");
        if (!phase || (std::string_view(phase) != "initializing" && std::string_view(phase) != "awaiting-foreground" && std::string_view(phase) != "warmup" &&
                       std::string_view(phase) != "sampling"))
            throw std::invalid_argument("Status: only collection phases are caller-controlled");
        WriteStatus(phase, elapsed);
    }
    void Session::WriteStatus(const char* phase, double elapsed)
    {
        export_.path = directory_ / "status.yaml";
        Data::Value state;
        state["protocol_version"] = 2;
        state["phase"] = phase;
        state["elapsed_seconds"] = elapsed;
        const auto temporary = directory_ / "status.tmp";
        auto file = Output(temporary);
        file << Data::DumpYaml(state) << '\n';
        file.close();
        if (export_probe) export_probe(export_probe_context, directory_, phase);
        try { Files::Publish(temporary, directory_ / "status.yaml"); }
        catch (const std::system_error& error) {
            throw std::system_error(error.code(), std::string("Benchmark status phase=") + phase
                                    + "; " + error.what());
        }
    }
    void Session::Add(Frame frame)
    {
        RequireRecording("Add frame");
        if (frames_.size() == 1000000) throw std::runtime_error("Benchmark frame capacity exceeded; partial data retained");
        if (frame.gpu_draw_ms) ValidateDuration(*frame.gpu_draw_ms);
        if (frame.cpu_prepare_ms) ValidateDuration(*frame.cpu_prepare_ms);
        if (frame.cpu_submit_ms) ValidateDuration(*frame.cpu_submit_ms);
        if (frame.gpu_draw_ms && !frame.gpu_missing_reason.empty())
            throw std::invalid_argument("GPU duration cannot also have a missing reason");
        if (frame.frame_id && renderer_frames_.contains(*frame.frame_id))
            throw std::logic_error("Duplicate renderer frame identity");
        if (!frame.focused && !allow_unfocused_) Invalidate("window-not-focused");
        const auto identity = frame.frame_id;
        const auto resolved = frame.gpu_draw_ms.has_value() || !frame.gpu_missing_reason.empty();
        frames_.push_back(std::move(frame));
        try {
            gpu_resolved_.push_back(resolved);
            if (identity) renderer_frames_.emplace(*identity, frames_.size() - 1);
        } catch (...) {
            frames_.pop_back();
            if (gpu_resolved_.size() > frames_.size()) gpu_resolved_.pop_back();
            throw;
        }
    }
    void Session::SetGpu(std::size_t frame, double milliseconds)
    {
        RequireRecording("SetGpu");
        if (frame >= frames_.size()) throw std::logic_error("GPU result has no matching CPU frame");
        ValidateDuration(milliseconds);
        frames_[frame].gpu_draw_ms = milliseconds;
        frames_[frame].gpu_missing_reason.clear();
        gpu_resolved_[frame] = true;
    }
    void Session::SetGpuSample(std::uint64_t frame_id, std::optional<double> milliseconds,
                               const std::string& missing_reason)
    {
        RequireRecording("SetGpuSample");
        if (milliseconds) ValidateDuration(*milliseconds);
        if ((milliseconds && !missing_reason.empty()) || (!milliseconds && missing_reason.empty()))
            throw std::invalid_argument("GPU result requires either a duration or a missing reason");
        const auto found = renderer_frames_.find(frame_id);
        if (found == renderer_frames_.end()) throw std::logic_error("GPU result has no matching renderer frame identity");
        const auto row = found->second;
        if (gpu_resolved_[row]) throw std::logic_error("Duplicate GPU result for renderer frame identity");
        frames_[row].gpu_missing_reason = missing_reason;
        frames_[row].gpu_draw_ms = milliseconds;
        gpu_resolved_[row] = true;
    }
    void Session::SetRendererInfo(const RendererInfo& info)
    {
        RequireRecording("SetRendererInfo");
        if (info.backend.empty() || info.actual_present_mode.empty() || info.gpu_time_scope.empty())
            throw std::invalid_argument("Renderer telemetry requires backend, actual present mode and GPU timing scope");
        Data::Value renderer;
        renderer["backend"] = info.backend;
        renderer["device_name"] = info.device_name;
        renderer["device_identity"] = info.device_identity;
        renderer["driver_version"] = info.driver_version;
        renderer["api_version"] = info.api_version;
        renderer["framebuffer_format"] = info.framebuffer_format;
        renderer["framebuffer_width"] = info.framebuffer_width;
        renderer["framebuffer_height"] = info.framebuffer_height;
        renderer["actual_samples"] = info.actual_samples;
        renderer["actual_present_mode"] = info.actual_present_mode;
        renderer["validation_enabled"] = info.validation_enabled;
        renderer["capabilities"]["texture_arrays"] = info.texture_arrays;
        renderer["capabilities"]["selection_boxes"] = info.selection_boxes;
        renderer["capabilities"]["screenshots"] = info.screenshots;
        renderer["capabilities"]["gpu_timestamps"] = info.gpu_timestamps;
        metadata["renderer"] = std::move(renderer);
        metadata["renderer_backend"] = info.backend;
        metadata["renderer_telemetry_version"] = 1;
        metadata["frame_time_scope"] = "consecutive Renderer::Render return intervals; not physical display latency";
        metadata["gpu_time_scope"] = info.gpu_time_scope;
        metadata["cpu_prepare_scope"] = "Renderer validation/preparation and backend frame-context reuse wait; excludes scene cache Create/Update";
        metadata["cpu_submit_scope"] = "backend command preparation/submission; not derived by subtracting upload time";
        metadata["upload_cpu_scope"] = "application scene-cache Synchronize CPU interval, including scan, owned-copy conversion, Create/Update and draw-list assembly; excludes Render CPU preparation/submission";
        metadata["upload_bytes_scope"] = "successful persistent-cache resource uploads counted once plus Render-local uploads; startup separately attributed";
        metadata["gpu_missing_sample_scope"] = "missing durations remain absent; unreturned IDs are not-returned at export";
        metadata["memory_scope"] = "process working set/private bytes; optional device-wide estimates are not process VRAM";
        if (!renderer_info_set_) renderer_frames_.reserve(100000);
        renderer_info_set_ = true;
    }
    void Session::AddStartupUpload(UploadStatistics statistics)
    {
        RequireRecording("AddStartupUpload");
        ValidateDuration(statistics.cpu_ms);
        const UploadStatistics combined{startup_upload_.cpu_ms + statistics.cpu_ms,
                                        AddCounter(startup_upload_.bytes, statistics.bytes),
                                        AddCounter(startup_upload_.calls, statistics.calls)};
        ValidateDuration(combined.cpu_ms);
        startup["resource_upload_cpu"] = combined.cpu_ms;
        metadata["startup_upload_bytes"] = combined.bytes;
        metadata["startup_upload_calls"] = combined.calls;
        startup_upload_ = combined;
    }
    void Session::Invalidate(const std::string& reason)
    {
        RequireRecording("Invalidate");
        if (std::find(invalid_reasons_.begin(), invalid_reasons_.end(), reason) == invalid_reasons_.end())
            invalid_reasons_.push_back(reason);
    }
    void Session::RequireRecording(const char* operation) const {
        if (state_ != RecordState::Recording)
            throw std::logic_error(std::string(operation) + ": performance record is sealed");
    }
    void Session::Add(Memory memory) { RequireRecording("Add memory"); memory_.push_back(memory); }
    void Session::SetMetadata(std::string_view key, Data::Value value) {
        RequireRecording("SetMetadata"); metadata[key] = std::move(value);
    }
    void Session::SetStartup(std::string_view key, Data::Value value) {
        RequireRecording("SetStartup"); startup[key] = std::move(value);
    }
    void Session::Complete() { RequireRecording("Complete"); completed = true; }
    void Session::Seal(const FinalFacts& facts) {
        if (state_ == RecordState::Sealed) {
            if (*final_ != facts) throw std::logic_error("Seal: conflicting final facts");
            return;
        }
        if (facts.completed != completed) throw std::logic_error("Seal: completion fact differs from the recorded outcome");
        auto reasons = invalid_reasons_;
        const auto add = [&](const char* reason) {
            if (std::find(reasons.begin(), reasons.end(), reason) == reasons.end()) reasons.emplace_back(reason);
        };
        if (!facts.runtime_succeeded) add("runtime-error");
        if (!facts.shutdown_succeeded) add("shutdown-error");
        if (!completed) add("duration-not-completed");
        auto candidate = std::make_unique<FinalFacts>(facts);
        final_ = std::move(candidate);
        invalid_reasons_.swap(reasons);
        state_ = RecordState::Sealed;
    }
    ExportResult Session::Export() {
        if (state_ != RecordState::Sealed) throw std::logic_error("Export: record must be sealed");
        if (export_.state == ExportState::Saved) return export_;
        export_ = {ExportState::Failed, false, directory_, "exporting", {}, {}};
        try {
            const bool valid = WriteExport();
            export_.valid_run = valid;
            export_.state = ExportState::Saved;
        } catch (const std::system_error& error) {
            export_.native_error = error.code();
            export_.diagnostic = error.what();
        } catch (const std::exception& error) {
            export_.diagnostic = error.what();
        } catch (...) {
            export_.diagnostic = "Unknown export failure";
        }
        return export_;
    }
    bool Session::WriteExport()
    {
        WriteStatus("exporting", 0);
        const bool normal_exit = final_->runtime_succeeded && final_->shutdown_succeeded;
        const bool renderer_rows = renderer_info_set_ || std::any_of(frames_.begin(), frames_.end(), [](const Frame& frame) {
            return frame.frame_id.has_value() || frame.cpu_prepare_ms.has_value() || frame.cpu_submit_ms.has_value();
        });
        export_.stage = "frames"; export_.path = directory_ / "frames.csv";
        auto csv = Output(export_.path);
        csv << "frame,phase,elapsed_s,frame_ms,simulation_delta_ms,event_ms,simulation_ms,mesh_ms,pack_ms,render_cpu_ms,upload_cpu_ms,present_ms,instrumentation_ms,gpu_draw_ms,rebuilt_chunks,vertices,upload_bytes,draw_calls,edits,edit_lateness_ms,x,y,z,yaw,focused";
        if (renderer_rows) csv << ",renderer_frame_id,cpu_prepare_ms,cpu_submit_ms,gpu_missing_reason,resource_upload_bytes,render_upload_bytes,upload_calls";
        csv << '\n';
        std::vector<double> frame_times, gpu_times, mesh_times, upload_times, submit_times, present_times, prepare_times, native_submit_times;
        std::map<std::string, std::uint64_t> missing_gpu_reasons;
        std::uint64_t sample_upload_bytes = 0, sample_resource_upload_bytes = 0, sample_render_upload_bytes = 0, sample_upload_calls = 0;
        std::uint64_t edits = 0, rebuilt = 0;
        std::uint64_t unfocused_frames = 0, sample_unfocused_frames = 0, transitions = 0;
        double unfocused_ms = 0, sample_unfocused_ms = 0;
        export_.stage = "focus"; export_.path = directory_ / "focus.csv";
        auto focus = Output(export_.path);
        focus << "frame,elapsed_s,focused\n";
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            const auto& f = frames_[i];
            if (i == 0 || frames_[i - 1].focused != f.focused) {
                export_.stage = "focus"; export_.path = directory_ / "focus.csv";
                focus << i << ',' << f.elapsed << ',' << f.focused << '\n';
                if (i != 0) ++transitions;
            }
            if (!f.focused) {
                ++unfocused_frames; unfocused_ms += f.frame_ms;
                if (f.measured) { ++sample_unfocused_frames; sample_unfocused_ms += f.frame_ms; }
            }
            export_.stage = "frames"; export_.path = directory_ / "frames.csv";
            csv << i << ',' << (f.measured ? "sample" : "warmup") << ',' << f.elapsed << ',' << f.frame_ms << ','
                << f.simulation_delta_ms << ',' << f.event_ms << ',' << f.simulation_ms << ',' << f.mesh_ms << ','
                << f.pack_ms << ',' << f.render_cpu_ms << ',' << f.upload_cpu_ms << ',' << f.present_ms << ',' << f.instrumentation_ms << ',';
            Optional(csv, f.gpu_draw_ms);
            csv << ',' << f.rebuilt_chunks << ',' << f.vertices << ',' << f.upload_bytes << ',' << f.draw_calls << ','
                << f.edits << ',' << f.edit_lateness_ms << ',' << f.x << ',' << f.y << ',' << f.z << ',' << f.yaw << ',' << f.focused;
            const auto missing_reason = f.gpu_draw_ms ? std::string{} : (!f.gpu_missing_reason.empty() ? f.gpu_missing_reason
                : (f.frame_id ? "not-returned" : "not-collected"));
            if (renderer_rows) {
                csv << ','; Optional(csv, f.frame_id);
                csv << ','; Optional(csv, f.cpu_prepare_ms);
                csv << ','; Optional(csv, f.cpu_submit_ms);
                csv << ','; CsvString(csv, missing_reason);
                csv << ',' << f.resource_upload_bytes << ',' << f.render_upload_bytes << ',' << f.upload_calls;
            }
            csv << '\n';
            if (!f.measured) continue;
            frame_times.push_back(f.frame_ms); mesh_times.push_back(f.mesh_ms);
            upload_times.push_back(f.upload_cpu_ms); present_times.push_back(f.present_ms);
            if (!f.frame_id) submit_times.push_back(f.pack_ms + f.render_cpu_ms - f.upload_cpu_ms);
            if (f.cpu_prepare_ms) prepare_times.push_back(*f.cpu_prepare_ms);
            if (f.cpu_submit_ms) native_submit_times.push_back(*f.cpu_submit_ms);
            if (f.gpu_draw_ms) gpu_times.push_back(*f.gpu_draw_ms);
            else if (renderer_rows) ++missing_gpu_reasons[missing_reason];
            if (renderer_rows) {
                sample_upload_bytes = AddCounter(sample_upload_bytes, f.upload_bytes);
                sample_resource_upload_bytes = AddCounter(sample_resource_upload_bytes, f.resource_upload_bytes);
                sample_render_upload_bytes = AddCounter(sample_render_upload_bytes, f.render_upload_bytes);
                sample_upload_calls = AddCounter(sample_upload_calls, f.upload_calls);
            }
            edits += f.edits; rebuilt += f.rebuilt_chunks;
        }
        export_.stage = "frames"; export_.path = directory_ / "frames.csv"; csv.close();
        export_.stage = "focus"; export_.path = directory_ / "focus.csv"; focus.close();
        export_.stage = "memory"; export_.path = directory_ / "memory.csv";
        auto memory = Output(export_.path);
        memory << "elapsed_s,working_set_bytes,private_bytes,device_dedicated_kib,device_available_kib\n";
        for (const auto& m : memory_) {
            memory << m.elapsed << ','; Optional(memory, m.working_set_bytes); memory << ',';
            Optional(memory, m.private_bytes); memory << ','; Optional(memory, m.device_dedicated_kib); memory << ',';
            Optional(memory, m.device_available_kib); memory << '\n';
        }
        memory.close();
        Data::Value report;
        report["metadata"] = metadata;
        report["startup_ms"] = startup;
        report["completed"] = completed;
        report["lifecycle"]["runtime_succeeded"] = final_->runtime_succeeded;
        report["lifecycle"]["shutdown_succeeded"] = final_->shutdown_succeeded;
        report["lifecycle"]["first_error"] = final_->first_error;
        report["lifecycle"]["secondary_errors"] = final_->secondary_errors;
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
        if (renderer_rows) {
            report["cpu_prepare_ms"] = Statistics(prepare_times);
            report["cpu_submit_ms"] = Statistics(native_submit_times);
            for (const auto& [reason, count] : missing_gpu_reasons)
                report["gpu_missing_sample_reasons"][reason] = count;
            report["sample_upload_bytes"] = sample_upload_bytes;
            report["sample_resource_upload_bytes"] = sample_resource_upload_bytes;
            report["sample_render_upload_bytes"] = sample_render_upload_bytes;
            report["sample_upload_calls"] = sample_upload_calls;
        }
        report["sample_edits"] = edits;
        report["sample_rebuilt_chunks"] = rebuilt;
        const double duration = std::accumulate(frame_times.begin(), frame_times.end(), 0.0);
        if (duration > 0) report["throughput_fps"] = frame_times.size() * 1000.0 / duration;
        export_.stage = "summary"; export_.path = directory_ / "summary.yaml";
        auto yaml = Output(export_.path);
        yaml << Data::DumpYaml(report) << '\n';
        yaml.close();
        export_.stage = "finished";
        WriteStatus("finished", 0);
        return report["valid_run"].as<bool>();
    }
}
