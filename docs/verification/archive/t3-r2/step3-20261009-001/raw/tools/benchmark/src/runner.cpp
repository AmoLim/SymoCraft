#include "platform.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>
#include <thread>

namespace Benchmark {
    namespace {
        constexpr const char* assets[]{"configs/blockFormats.yaml", "shaders/vs_BlockShader.glsl", "shaders/fs_BlockShader.glsl",
            "shaders/vs_FrameShader.glsl", "shaders/fs_FrameShader.glsl", "textures/texture_atlas.png"};
        void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
        YAML::Node Identity(const Config& config)
        {
            YAML::Node node;
            node["game_sha256"] = Sha256(config.game);
            for (const auto asset : assets) node["asset_sha256"][asset] = Sha256(config.game.parent_path() / "assets" / asset);
            return node;
        }
        YAML::Node Settings(const Config& config)
        {
            YAML::Node node;
            node["profile"] = config.quick ? "quick-60s" : "baseline-36m";
            node["focus_policy"] = config.focus_policy;
            node["machine_label"] = config.machine_label; node["power_mode_declared"] = config.power_mode;
            node["gpu_mode_declared"] = config.gpu_mode; node["clock_settings_declared"] = config.clocks;
            node["framebuffer_width"] = 1920; node["framebuffer_height"] = 1080;
            node["seed"] = 424242; node["vsync"] = false;
            return node;
        }
        std::vector<std::string_view> Columns(const std::string& line)
        {
            std::vector<std::string_view> result;
            std::string_view rest(line);
            if (!rest.empty() && rest.back() == '\r') rest.remove_suffix(1);
            for (;;) {
                const auto end = rest.find(','); result.push_back(rest.substr(0, end));
                if (end == std::string_view::npos) return result;
                rest.remove_prefix(end + 1);
            }
        }
        double Number(std::string_view value)
        {
            double number = 0;
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
            Require(parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size() && std::isfinite(number), "Invalid numeric CSV value");
            return number;
        }
        void Same(double actual, double expected, const char* message)
        { Require(std::isfinite(actual) && std::abs(actual - expected) <= std::max(0.0001, std::abs(expected) * 0.00001), message); }
        void ValidateFrames(const fs::path& capture, const Round& round, const std::string& policy, const YAML::Node& summary)
        {
            RequireRegular(capture / "frames.csv");
            std::ifstream input(capture / "frames.csv"); std::string line;
            Require(static_cast<bool>(std::getline(input, line)), "Empty frames CSV");
            Require(line == "frame,phase,elapsed_s,frame_ms,simulation_delta_ms,event_ms,simulation_ms,mesh_ms,pack_ms,render_cpu_ms,upload_cpu_ms,present_ms,instrumentation_ms,gpu_draw_ms,rebuilt_chunks,vertices,upload_bytes,draw_calls,edits,edit_lateness_ms,x,y,z,yaw,focused", "Unsupported frames CSV header");
            std::size_t rows = 0, unfocused = 0, sample_unfocused = 0; double previous = -1, last_frame = 0;
            double unfocused_ms = 0, sample_unfocused_ms = 0;
            std::vector<double> times; std::uint64_t edits = 0;
            while (std::getline(input, line)) {
                const auto columns = Columns(line); Require(columns.size() == 25, "Truncated frames CSV");
                Require(Number(columns[0]) == static_cast<double>(rows++), "Discontinuous frame indices");
                const auto elapsed = Number(columns[2]); const auto ms = Number(columns[3]);
                Require(elapsed >= 0 && elapsed >= previous && ms >= 0, "Invalid frame order/timing");
                const bool sample = elapsed >= round.warmup;
                Require(columns[1] == (sample ? "sample" : "warmup"), "Wrong frame phase");
                Require(columns[24] == "0" || columns[24] == "1", "Invalid focus value");
                if (columns[24] == "0") {
                    Require(policy == "allow-unfocused", "Strict capture lost focus");
                    ++unfocused; unfocused_ms += ms;
                    if (sample) { ++sample_unfocused; sample_unfocused_ms += ms; }
                }
                if (sample) {
                    times.push_back(ms);
                    const auto count = Number(columns[18]);
                    Require(count >= 0 && count <= 100000 && std::floor(count) == count, "Invalid edit count");
                    edits += static_cast<std::uint64_t>(count);
                }
                previous = elapsed; last_frame = ms;
                Require(rows <= 1000000, "Oversized frame capture");
            }
            Require(input.eof() && !times.empty(), "Missing sample frames");
            Require(previous + last_frame / 1000 >= round.warmup + round.sample - 0.5, "Capture duration is too short");
            Require(rows == summary["total_frames"].as<std::size_t>() && times.size() == summary["frame_ms"]["count"].as<std::size_t>(), "Frame count mismatch");
            Require(edits == summary["sample_edits"].as<std::uint64_t>(), "Edit count mismatch");
            Require(unfocused == summary["focus"]["unfocused_frames"].as<std::size_t>() && sample_unfocused == summary["focus"]["sample_unfocused_frames"].as<std::size_t>(), "Focus count mismatch");
            Same(summary["focus"]["unfocused_seconds_estimate"].as<double>(), unfocused_ms / 1000, "Focus duration mismatch");
            Same(summary["focus"]["sample_unfocused_seconds_estimate"].as<double>(), sample_unfocused_ms / 1000, "Sample focus duration mismatch");
            std::sort(times.begin(), times.end());
            for (const auto [key, quantile] : {std::pair{"p50", .5}, {"p95", .95}, {"p99", .99}})
                Same(summary["frame_ms"][key].as<double>(), times[static_cast<std::size_t>(std::ceil(quantile * times.size())) - 1], "Percentile mismatch");
            const auto sum = std::accumulate(times.begin(), times.end(), 0.0);
            Require(sum > 0, "Zero measured time");
            Same(summary["frame_ms"]["mean"].as<double>(), sum / times.size(), "Mean mismatch");
            Same(summary["throughput_fps"].as<double>(), times.size() * 1000 / sum, "Throughput mismatch");
        }
        void Notify(const Observer& observer, const Progress& progress) { if (observer) observer(progress); }
        void Redact(std::string& text, const std::wstring& path)
        {
            if (path.empty()) return;
            auto replace = [&](std::string match) {
                if (match.empty()) return;
                for (std::size_t at = 0; (at = text.find(match, at)) != std::string::npos;) { text.replace(at, match.size(), "[local-path]"); at += 12; }
            };
            auto match = Utf8(path); replace(match);
            std::replace(match.begin(), match.end(), '\\', '/'); replace(match);
        }
    }
    std::vector<Round> Plan(bool quick)
    {
        std::vector<Round> plan;
        for (const auto scene : {"static", "walk", "edit"})
            for (unsigned repeat = 1; repeat <= (quick ? 1u : 3u); ++repeat)
                plan.push_back({scene, repeat, quick ? 10u : 60u, quick ? 10u : 180u});
        return plan;
    }
    YAML::Node ValidateCapture(const fs::path& capture, const Round& round, const std::string& policy, unsigned long exit_code)
    {
        const auto summary = ReadYaml(capture / "summary.yaml");
        Require(exit_code == 0, "Game returned a nonzero exit code; inspect stderr.log and summary.yaml");
        Require(summary["completed"].as<bool>() && summary["valid_run"].as<bool>(), "Game marked capture incomplete or invalid");
        Require(summary["invalid_reasons"].IsSequence() && summary["invalid_reasons"].size() == 0, "Game reported invalid reasons");
        const auto meta = summary["metadata"];
        Require(meta["schema_version"].as<int>() == 2 && meta["protocol_version"].as<int>() == 2 && meta["workload_version"].as<int>() == 2, "Incompatible game protocol/workload; update the whole package");
        Require(meta["benchmark"].as<std::string>() == round.scene && meta["focus_policy"].as<std::string>() == policy, "Capture mode/policy mismatch");
        Require(meta["warmup_seconds"].as<unsigned>() == round.warmup && meta["sample_seconds"].as<unsigned>() == round.sample, "Capture timing configuration mismatch");
        Require(meta["framebuffer_width"].as<int>() == 1920 && meta["framebuffer_height"].as<int>() == 1080 && !meta["requested_vsync"].as<bool>(), "Capture rendering configuration mismatch");
        Require(meta["world"]["generation"]["seed"].as<unsigned>() == 424242 && meta["world"]["chunks"].as<int>() == 441, "Capture world configuration mismatch");
        Require(meta["world"]["terrain_digest"].as<std::string>() == "fnv1a64:cd80ebb0446c15c6" && meta["world"]["scene_digest"].as<std::string>() == "fnv1a64:bddd435ea737fa62", "Capture world digest mismatch");
        Require(!meta["gl_renderer"].as<std::string>().empty() && !meta["gl_version"].as<std::string>().empty(), "Missing graphics device identification");
        for (const auto file : {"memory.csv", "focus.csv"}) {
            RequireRegular(capture / file); Require(fs::file_size(capture / file) > 0, "Empty capture artifact");
        }
        Require(ReadYaml(capture / "status.yaml")["phase"].as<std::string>() == "finished", "Missing terminal game status");
        ValidateFrames(capture, round, policy, summary);
        return summary;
    }
    Outcome Run(const Config& config, std::stop_token stop, const Observer& observer, bool resume)
    {
        Require(config.game.is_absolute() && config.output.is_absolute(), "Runner paths must be absolute");
        Require(config.focus_policy == "strict" || config.focus_policy == "allow-unfocused", "Unknown focus policy");
        const auto identity = Identity(config); const auto settings = Settings(config);
        const auto plan = Plan(config.quick);
        YAML::Node session;
        if (resume) {
            session = ReadYaml(config.output / "session.yaml");
            Require(session["runner_schema"].as<int>() == 1 && session["protocol_version"].as<int>() == 2, "Unsupported session version");
            Require(YAML::Dump(session["identity"]) == YAML::Dump(identity) && YAML::Dump(session["settings"]) == YAML::Dump(settings), "Cannot retry after package/settings changed");
            session["retried"] = true;
        } else {
            fs::create_directories(config.output.parent_path());
            Require(fs::create_directory(config.output), "Output already exists; use a new directory or Retry");
            session["runner_schema"] = 1; session["protocol_version"] = 2;
            session["created_utc"] = Timestamp(); session["identity"] = identity;
            session["runner_sha256"] = Sha256(ExecutablePath()); session["settings"] = settings;
            session["machine"] = Machine(); session["retried"] = false;
            session["rounds"] = YAML::Node(YAML::NodeType::Sequence);
            for (const auto& round : plan) {
                YAML::Node item; item["scene"] = round.scene; item["repeat"] = round.repeat;
                item["passed"] = false; item["attempts"] = 0; session["rounds"].push_back(item);
            }
        }
        Require(session["rounds"].size() == plan.size(), "Unexpected session plan");
        // The lock is not inherited. A crash releases it; concurrent runners cannot append to this session.
        Handle lock(CreateFileW((config.output / "session.lock").c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        Check(lock.value != INVALID_HANDLE_VALUE, "Lock benchmark session");
        Outcome outcome; outcome.directory = config.output;
        Progress progress; progress.total = static_cast<unsigned>(plan.size());
        session["completed"] = false; WriteYaml(config.output / "session.yaml", session);
        for (unsigned index = 0; index < plan.size(); ++index) {
            const auto& round = plan[index]; auto item = session["rounds"][index];
            Require(item["scene"].as<std::string>() == round.scene && item["repeat"].as<unsigned>() == round.repeat, "Session round mismatch");
            if (item["passed"].as<bool>()) {
                const auto previous_attempt = item["attempts"].as<unsigned>();
                Require(previous_attempt > 0 && previous_attempt <= 1000, "Invalid previous attempt");
                const auto previous = config.output / (round.scene + "-" + std::to_string(round.repeat) + "-attempt-" + std::to_string(previous_attempt));
                const auto result = ReadYaml(previous / "result.yaml");
                Require(result["passed"].as<bool>(), "Previously passed result changed");
                ValidateCapture(previous / "capture", round, config.focus_policy, result["exit_code"].as<unsigned long>());
                ++outcome.completed; continue;
            }
            if (stop.stop_requested()) { outcome.cancelled = true; break; }
            const unsigned attempt = item["attempts"].as<unsigned>() + 1;
            Require(attempt <= 1000, "Too many attempts"); item["attempts"] = attempt;
            const auto name = round.scene + "-" + std::to_string(round.repeat) + "-attempt-" + std::to_string(attempt);
            const auto directory = config.output / name;
            Require(fs::create_directory(directory), "Attempt directory already exists");
            item["directory"] = name; item["state"] = "running";
            WriteYaml(config.output / "session.yaml", session);
            progress.round = index + 1; progress.passed = outcome.completed; progress.duration = round.warmup + round.sample;
            progress.phase = "initializing"; progress.elapsed = 0; progress.message = name; Notify(observer, progress);
            YAML::Node result; result["runner_schema"] = 1; result["started_utc"] = Timestamp(); result["machine"] = Machine();
            bool passed = false, cancelled = false;
            try {
                Require(YAML::Dump(Identity(config)) == YAML::Dump(identity), "Package changed during the session");
                Process process(config.game, {L"--benchmark", Wide(round.scene), L"--output", (directory / "capture").wstring(), L"--seed", L"424242",
                    L"--width", L"1920", L"--height", L"1080", L"--vsync", L"0", L"--warmup-seconds", std::to_wstring(round.warmup),
                    L"--sample-seconds", std::to_wstring(round.sample), L"--focus-policy", Wide(config.focus_policy)},
                    config.game.parent_path(), directory / "stdout.log", directory / "stderr.log");
                const auto start = Clock::now(); auto phase_start = start;
                std::string phase; double phase_elapsed = 0;
                bool timeout = false, forced = false;
                while (process.Running()) {
                    const auto now = Clock::now();
                    timeout = now - start > std::chrono::seconds(round.warmup + round.sample + 120);
                    cancelled = stop.stop_requested();
                    if (timeout || cancelled) { forced = process.Stop(); break; }
                    try {
                        const auto status = ReadYaml(directory / "capture/status.yaml");
                        if (status["protocol_version"].as<int>() == 2) {
                            const auto next = status["phase"].as<std::string>();
                            if (next != phase) { phase = next; phase_start = now; phase_elapsed = status["elapsed_seconds"].as<double>(); }
                        }
                    } catch (const std::exception&) { /* Atomic publication may not exist during startup. */ }
                    progress.phase = phase.empty() ? "initializing" : phase;
                    progress.elapsed = phase == "warmup" || phase == "sampling"
                        ? std::min(progress.duration, phase_elapsed + std::chrono::duration<double>(now - phase_start).count()) : 0;
                    Notify(observer, progress);
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
                result["exit_code"] = process.ExitCode(); result["cancelled"] = cancelled;
                result["timed_out"] = timeout; result["forced_termination"] = forced;
                Require(!cancelled && !timeout, cancelled ? "Cancelled; partial evidence retained" : "Game timed out; partial evidence retained");
                const auto summary = ValidateCapture(directory / "capture", round, config.focus_policy, process.ExitCode());
                Require(YAML::Dump(Identity(config)) == YAML::Dump(identity), "Package changed during capture");
                const auto device = summary["metadata"]["gl_renderer"].as<std::string>() + " | " + summary["metadata"]["gl_version"].as<std::string>();
                if (session["graphics_device"]) Require(session["graphics_device"].as<std::string>() == device, "Graphics device changed between rounds");
                else session["graphics_device"] = device;
                session["build_configuration"] = summary["metadata"]["build_configuration"];
                result["frame_ms"] = summary["frame_ms"]; result["throughput_fps"] = summary["throughput_fps"];
                result["focus"] = summary["focus"]; result["gl_renderer"] = summary["metadata"]["gl_renderer"];
                result["build_configuration"] = summary["metadata"]["build_configuration"];
                passed = true;
            } catch (const std::exception& error) { result["error"] = error.what(); }
            result["passed"] = passed; result["finished_utc"] = Timestamp();
            WriteYaml(directory / "result.yaml", result);
            item["passed"] = passed; item["state"] = passed ? "passed" : cancelled ? "cancelled" : "failed";
            if (passed) ++outcome.completed;
            outcome.cancelled = cancelled; session["completed_rounds"] = outcome.completed;
            WriteYaml(config.output / "session.yaml", session);
            progress.passed = outcome.completed; progress.phase = item["state"].as<std::string>();
            progress.message = passed ? name + " passed" : name + ": " + result["error"].as<std::string>(); Notify(observer, progress);
            if (!passed) break;
        }
        outcome.passed = outcome.completed == plan.size();
        session["completed"] = outcome.passed; session["completed_rounds"] = outcome.completed;
        session["cancelled"] = outcome.cancelled; session["updated_utc"] = Timestamp();
        session["continuous_full_protocol"] = outcome.passed && !config.quick && !session["retried"].as<bool>();
        session["comparison_note"] = "Compare only matching game/assets, workload, focus policy, profile and environment. Background applications may contend for CPU/GPU.";
        WriteYaml(config.output / "session.yaml", session);
        return outcome;
    }
    fs::path Export(const Config& config, std::stop_token stop)
    {
        const auto session = ReadYaml(config.output / "session.yaml");
        Handle lock(CreateFileW((config.output / "session.lock").c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        Check(lock.value != INVALID_HANDLE_VALUE, "Lock session for export");
        const auto export_root = config.output.parent_path() / ("export-" + Timestamp());
        Require(fs::create_directory(export_root), "Export directory exists");
        const auto package = export_root / "results"; fs::create_directory(package);
        YAML::Node hashes;
        wchar_t home[32768]{}; GetEnvironmentVariableW(L"USERPROFILE", home, 32768);
        auto copy = [&](const fs::path& relative) {
            if (stop.stop_requested()) throw std::runtime_error("Export cancelled; partial files retained");
            const auto from = config.output / relative; RequireRegular(from);
            const auto to = package / relative; fs::create_directories(to.parent_path());
            if (relative.extension() == ".log" || relative.extension() == ".yaml") {
                Require(fs::file_size(from) < 16 * 1024 * 1024, "Oversized text artifact");
                std::ifstream input(from, std::ios::binary); std::string text((std::istreambuf_iterator<char>(input)), {});
                Redact(text, config.game.parent_path().wstring()); Redact(text, config.output.wstring()); Redact(text, home);
                std::ofstream output(to, std::ios::binary); output.exceptions(std::ios::failbit | std::ios::badbit); output << text;
            } else fs::copy_file(from, to);
            hashes[Utf8(relative.generic_wstring())] = Sha256(to);
        };
        copy("session.yaml");
        // Enumerate our fixed round/attempt names, never arbitrary paths from a result file.
        const auto rounds = Plan(config.quick);
        Require(session["rounds"].size() == rounds.size(), "Export session plan mismatch");
        for (unsigned i = 0; i < rounds.size(); ++i) {
            const auto attempts = session["rounds"][i]["attempts"].as<unsigned>(); Require(attempts <= 1000, "Invalid attempt count");
            for (unsigned a = 1; a <= attempts; ++a) {
                const fs::path directory = rounds[i].scene + "-" + std::to_string(rounds[i].repeat) + "-attempt-" + std::to_string(a);
                for (const auto file : {"result.yaml", "stdout.log", "stderr.log", "capture/summary.yaml", "capture/status.yaml",
                    "capture/frames.csv", "capture/memory.csv", "capture/focus.csv", "capture/final-frame.png"})
                    if (fs::exists(config.output / directory / file)) copy(directory / file);
            }
        }
        WriteYaml(package / "sha256.yaml", hashes);
        wchar_t system[MAX_PATH]{}; Check(GetSystemDirectoryW(system, MAX_PATH) != 0, "Locate system tar");
        const auto tar = fs::path(system) / "tar.exe"; RequireRegular(tar);
        const auto temporary = export_root / "results.partial.zip";
        Process process(tar, {L"-a", L"-c", L"-f", temporary.wstring(), L"-C", export_root.wstring(), L"results"},
            export_root, export_root / "archive.stdout.log", export_root / "archive.stderr.log");
        const auto start = Clock::now();
        while (process.Running()) {
            if (stop.stop_requested() || Clock::now() - start > std::chrono::minutes(3)) {
                process.Stop(); throw std::runtime_error("Archive cancelled/timed out; export folder retained");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        Require(process.ExitCode() == 0 && fs::file_size(temporary) > 0, "System tar failed; export folder retained");
        const auto archive = export_root / "results.zip"; fs::rename(temporary, archive);
        YAML::Node identity; identity["sha256"] = Sha256(archive); WriteYaml(export_root / "archive.yaml", identity);
        return archive;
    }
}
