#include "symocraft/assets/image.h"
#include "symocraft/foundation/document.h"
#include "symocraft/platform/window.h"
#include "symocraft/renderer/renderer.h"
#include "graphics_bridge/native_bridge.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <memory>
#include <numbers>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using Data::Value;
using Clock = std::chrono::steady_clock;
using Color = std::array<std::uint8_t, 4>;
constexpr Color Red{230, 20, 40, 255};
constexpr Color Green{20, 220, 50, 255};
constexpr Color Yellow{230, 200, 20, 255};
constexpr Color Blue{20, 40, 230, 255};

struct Options {
    std::filesystem::path output;
    std::filesystem::path shaders;
    std::filesystem::path recovery_shaders;
    std::filesystem::path atlas;
    std::string mode = "normal";
    std::string run_id;
    std::string expected_device_name = "NVIDIA GeForce RTX 5070 Ti";
    std::uint32_t samples = 1;
    bool vsync = false;
};

Options ParseOptions(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (++index == argc) throw std::invalid_argument("missing option value");
        const std::string value = argv[index];
        if (argument == "--output-dir") options.output = value;
        else if (argument == "--shader-root") options.shaders = value;
        else if (argument == "--recovery-shader-root") options.recovery_shaders = value;
        else if (argument == "--atlas") options.atlas = value;
        else if (argument == "--mode") options.mode = value;
        else if (argument == "--run-id") options.run_id = value;
        else if (argument == "--expected-device-name") options.expected_device_name = value;
        else if (argument == "--samples") {
            if (value != "1" && value != "4") throw std::invalid_argument("samples must be 1 or 4");
            options.samples = value == "1" ? 1U : 4U;
        } else if (argument == "--vsync") {
            if (value != "true" && value != "false") throw std::invalid_argument("vsync must be true or false");
            options.vsync = value == "true";
        } else throw std::invalid_argument("unknown option: " + std::string(argument));
    }
    if (options.output.empty() || options.shaders.empty() || options.atlas.empty() || options.run_id.empty())
        throw std::invalid_argument("output-dir, shader-root, atlas, and run-id are required");
    if (options.mode != "normal" && options.mode != "missing-shader" && options.mode != "bad-shader")
        throw std::invalid_argument("unknown mode");
    options.output = std::filesystem::absolute(options.output);
    options.shaders = std::filesystem::absolute(options.shaders);
    if (!options.recovery_shaders.empty()) options.recovery_shaders = std::filesystem::absolute(options.recovery_shaders);
    options.atlas = std::filesystem::absolute(options.atlas);
    if (std::filesystem::exists(options.output)) throw std::invalid_argument("output-dir must be new");
    if (options.mode != "normal" && options.recovery_shaders.empty())
        throw std::invalid_argument("constructor failure mode requires recovery-shader-root");
    std::filesystem::create_directories(options.output);
    return options;
}

void WriteJsonString(std::ostream& stream, std::string_view value) {
    constexpr char hexadecimal[] = "0123456789abcdef";
    stream.put('"');
    for (const unsigned char character : value) {
        if (character == '"' || character == '\\') { stream.put('\\'); stream.put(static_cast<char>(character)); }
        else if (character < 0x20) {
            stream << "\\u00" << hexadecimal[character >> 4] << hexadecimal[character & 0xf];
        } else stream.put(static_cast<char>(character));
    }
    stream.put('"');
}

// Serialize the owned document tree, including all RFC 8259 control escapes.
void WriteJson(std::ostream& stream, const Value& value) {
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) stream << "null";
        else if constexpr (std::is_same_v<T, bool>) stream << (item ? "true" : "false");
        else if constexpr (std::is_same_v<T, std::string>) WriteJsonString(stream, item);
        else if constexpr (std::is_same_v<T, Value::Sequence>) {
            stream.put('[');
            bool first = true;
            for (const auto& child : item) {
                if (!std::exchange(first, false)) stream.put(',');
                WriteJson(stream, child);
            }
            stream.put(']');
        } else if constexpr (std::is_same_v<T, Value::Mapping>) {
            stream.put('{');
            bool first = true;
            for (const auto& [key, child] : item) {
                if (!std::exchange(first, false)) stream.put(',');
                WriteJsonString(stream, key);
                stream.put(':');
                WriteJson(stream, child);
            }
            stream.put('}');
        } else if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(item)) throw std::invalid_argument("nonfinite JSON number");
            stream << std::setprecision(std::numeric_limits<T>::max_digits10) << item;
        } else stream << item;
    }, value.storage());
}

std::string Json(const Value& value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    WriteJson(stream, value);
    return stream.str();
}

const char* PresentModeName(PresentMode mode) {
    switch (mode) {
    case PresentMode::Immediate: return "Immediate";
    case PresentMode::Fifo: return "Fifo";
    case PresentMode::Mailbox: return "Mailbox";
    default: return "Unknown";
    }
}

Value ExtentDocument(Extent2D extent) {
    Value value;
    value["width"] = extent.width;
    value["height"] = extent.height;
    return value;
}

Value InfoDocument(const RendererInfo& info) {
    Value value;
    value["backend"] = info.backend == Backend::D3D12 ? "D3D12" : "other";
    value["device_name"] = info.device_name;
    value["device_identity"] = info.device_identity;
    value["driver_version"] = info.driver_version;
    value["api_version"] = info.api_version;
    value["framebuffer_format"] = info.framebuffer_format;
    value["framebuffer_extent"] = ExtentDocument(info.framebuffer_extent);
    value["actual_samples"] = info.actual_samples;
    value["actual_present_mode"] = PresentModeName(info.actual_present_mode);
    value["validation_enabled"] = info.validation_enabled;
    value["capabilities"]["texture_arrays"] = info.capabilities.texture_arrays;
    value["capabilities"]["selection_boxes"] = info.capabilities.selection_boxes;
    value["capabilities"]["screenshots"] = info.capabilities.screenshots;
    value["capabilities"]["gpu_timestamps"] = info.capabilities.gpu_timestamps;
    return value;
}

Value ErrorDocument(const RenderError& error) {
    Value value;
    value["category"] = static_cast<std::uint32_t>(error.Category());
    value["backend"] = error.BackendName();
    value["operation"] = error.Operation();
    value["native_code"] = error.NativeCode();
    value["diagnostic"] = error.Diagnostic();
    value["message"] = error.what();
    return value;
}

class Report {
public:
    explicit Report(const Options& options) : output_(options.output) {
        value["schema"] = "symocraft.d3d12-backend-smoke.v1";
        value["run_id"] = options.run_id;
        value["mode"] = options.mode;
        value["requested_samples"] = options.samples;
        value["requested_vsync"] = options.vsync;
        value["requested_validation"] = true;
        value["expected_device_name"] = options.expected_device_name;
        value["requested_wait_timeout_ms"] = 2500;
        value["shader_asset_root"] = options.shaders.generic_string();
        value["recovery_shader_asset_root"] = options.recovery_shaders.generic_string();
        value["atlas_source"] = options.atlas.generic_string();
        value["scene"] = "fixed-camera test quads plus decoded real atlas; not a world or gameplay";
        value["production_package_acceptance"] = false;
        value["device_lost_exercised"] = false;
        value["actual_in_flight_timeout_exercised"] = false;
        value["checks"] = Value::Sequence{};
        value["frames"] = Value::Sequence{};
        value["gpu_samples"] = Value::Sequence{};
        value["screenshots"] = Value::Sequence{};
        value["success"] = false;
        value["compiler"]["cplusplus"] = static_cast<std::uint64_t>(__cplusplus);
#ifdef _MSC_VER
        value["compiler"]["msc_ver"] = _MSC_VER;
        value["compiler"]["msc_full_ver"] = _MSC_FULL_VER;
        value["compiler"]["msc_lang"] = _MSVC_LANG;
#endif
#ifdef NDEBUG
        value["compiler"]["ndebug"] = true;
#else
        value["compiler"]["ndebug"] = false;
#endif
        std::string controls;
        for (int character = 0; character < 32; ++character) controls.push_back(static_cast<char>(character));
        value["json_emitter_probe"]["all_control_characters"] = controls;
        value["json_emitter_probe"]["quote_backslash"] = "\"\\";
        value["json_emitter_probe"]["unicode_utf8"] = "\xc3\xa9\xe4\xb8\xad";
        value["json_emitter_probe"]["null"] = Value{};
        bool rejected_nonfinite = false;
        try { static_cast<void>(Json(Value{std::numeric_limits<double>::quiet_NaN()})); }
        catch (const std::invalid_argument&) { rejected_nonfinite = true; }
        if (!rejected_nonfinite) throw std::runtime_error("JSON emitter accepted a nonfinite number");
        value["json_emitter_probe"]["nonfinite_rejected"] = rejected_nonfinite;
    }

    void Check(bool condition, std::string_view name, Value observations = {}) {
        Value row;
        row["name"] = name;
        row["passed"] = condition;
        row["observations"] = std::move(observations);
        value["checks"].push_back(std::move(row));
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        Save();
        if (!condition) throw std::runtime_error(std::string(name));
    }

    void Save() const {
        std::ofstream stream(output_ / "result.json", std::ios::binary | std::ios::trunc);
        if (!stream) throw std::runtime_error("cannot write result.json");
        stream << Json(value) << '\n';
        stream.close();
        if (!stream) throw std::runtime_error("cannot close result.json");
    }

    void Capture(std::string_view name, const Screenshot& screenshot) {
        const auto filename = std::string(name) + ".png";
        const auto path = output_ / filename;
        if (std::filesystem::exists(path)) throw std::runtime_error("screenshot would overwrite prior evidence");
        if (!stbi_write_png(path.string().c_str(), static_cast<int>(screenshot.extent.width),
                            static_cast<int>(screenshot.extent.height), 4, screenshot.pixels.data(),
                            static_cast<int>(screenshot.extent.width * 4U)))
            throw std::runtime_error("cannot write screenshot PNG");
        Value row;
        row["file"] = filename;
        row["extent"] = ExtentDocument(screenshot.extent);
        row["orientation"] = "top-left owned RGBA8; written without image modification";
        value["screenshots"].push_back(std::move(row));
    }

    Value value;
private:
    std::filesystem::path output_;
};

class NativeWindow {
public:
    NativeWindow() {
        Window::Init();
        try { window_.reset(Window::Create("SymoCraft D3D12 backend verification", 640, 480, false, true, WindowMode::Native)); }
        catch (...) { Window::Free(); throw; }
    }
    ~NativeWindow() noexcept {
        window_.reset();
        try { Window::Free(); }
        catch (const std::exception& error) { std::cerr << "Window::Free cleanup: " << error.what() << '\n'; }
    }
    Window& Get() const { return *window_; }
private:
    std::unique_ptr<Window> window_;
};

Extent2D Drawable(Window& window) {
    window.PollInt();
    if (window.Minimized()) return {};
    return {static_cast<std::uint32_t>(std::max(window.width, 0)),
            static_cast<std::uint32_t>(std::max(window.height, 0))};
}

TextureArrayData Fixture(ColorSpace color_space = ColorSpace::Linear) {
    TextureArrayData texture;
    texture.width = texture.height = 2;
    texture.layers = 2;
    texture.color_space = color_space;
    for (const auto& pixel : {Red, Color{255, 0, 255, 0}, Green, Yellow, Blue, Blue, Blue, Blue})
        texture.pixels.insert(texture.pixels.end(), pixel.begin(), pixel.end());
    return texture;
}

TextureArrayData GrayFixture(ColorSpace color_space) {
    auto texture = Fixture(color_space);
    for (std::size_t offset = 0; offset < texture.pixels.size(); offset += 4) {
        texture.pixels[offset] = texture.pixels[offset + 1] = texture.pixels[offset + 2] = 64;
        texture.pixels[offset + 3] = 255;
    }
    return texture;
}

TextureArrayData NormalizeAtlas(const Assets::Image& image) {
    constexpr std::uint32_t tile = 64;
    if (image.width <= 0 || image.height <= 0 || image.width % tile || image.height % tile ||
        image.channels < 1 || image.channels > 4)
        throw std::invalid_argument("atlas must use complete 64px tiles and 1..4 source channels");
    TextureArrayData texture;
    texture.width = texture.height = tile;
    const auto columns = static_cast<std::uint32_t>(image.width) / tile;
    const auto rows = static_cast<std::uint32_t>(image.height) / tile;
    texture.layers = columns * rows;
    texture.pixels.resize(static_cast<std::size_t>(tile) * tile * texture.layers * 4);
    for (std::uint32_t row = 0; row < rows; ++row)
        for (std::uint32_t column = 0; column < columns; ++column)
            for (std::uint32_t y = 0; y < tile; ++y)
                for (std::uint32_t x = 0; x < tile; ++x) {
                    const auto source = (static_cast<std::size_t>(row * tile + y) * image.width + column * tile + x) * image.channels;
                    const auto layer = (rows - 1 - row) * columns + column;
                    const auto target = ((static_cast<std::size_t>(layer) * tile + y) * tile + x) * 4;
                    const auto grayscale = image.channels <= 2;
                    texture.pixels[target] = image.pixels.at(source);
                    texture.pixels[target + 1] = image.pixels.at(source + (grayscale ? 0 : 1));
                    texture.pixels[target + 2] = image.pixels.at(source + (grayscale ? 0 : 2));
                    texture.pixels[target + 3] = image.channels == 2 ? image.pixels.at(source + 1)
                        : image.channels == 4 ? image.pixels.at(source + 3) : 255;
                }
    return texture;
}

MeshData Quad(float layer, bool reversed = false, std::optional<glm::vec2> constant_uv = {}) {
    const std::array<glm::ivec3, 4> positions{{{-1, -1, 0}, {1, -1, 0}, {1, 1, 0}, {-1, 1, 0}}};
    const std::array<glm::vec2, 4> uv{{{0, 1}, {1, 1}, {1, 0}, {0, 0}}};
    std::array<std::size_t, 6> indices{0, 1, 2, 0, 2, 3};
    if (reversed) { std::swap(indices[1], indices[2]); std::swap(indices[4], indices[5]); }
    MeshData mesh;
    for (const auto index : indices) {
        const auto coordinate = constant_uv.value_or(uv[index]);
        mesh.vertices.push_back({positions[index], {coordinate.x, coordinate.y, layer}, 1.0f});
    }
    return mesh;
}

MeshDraw Draw(MeshHandle mesh, TextureArrayHandle texture, float depth = -3.0f, float scale = 1.0f) {
    MeshDraw draw;
    draw.mesh = mesh;
    draw.texture_array = texture;
    draw.object_transform = glm::mat4{1.0f};
    draw.object_transform[0][0] = draw.object_transform[1][1] = scale;
    draw.object_transform[3][2] = depth;
    return draw;
}

Color Pixel(const Screenshot& screenshot, std::uint32_t x, std::uint32_t y) {
    if (x >= screenshot.extent.width || y >= screenshot.extent.height) throw std::invalid_argument("pixel out of range");
    const auto offset = (static_cast<std::size_t>(y) * screenshot.extent.width + x) * 4;
    return {screenshot.pixels.at(offset), screenshot.pixels.at(offset + 1),
            screenshot.pixels.at(offset + 2), screenshot.pixels.at(offset + 3)};
}

Color WorldPixel(const Screenshot& screenshot, float x, float y, float depth = 3.0f) {
    const auto aspect = static_cast<float>(screenshot.extent.AspectRatio());
    const auto pixel_x = static_cast<std::uint32_t>((1.0f + x / (depth * aspect)) * screenshot.extent.width * 0.5f);
    const auto pixel_y = static_cast<std::uint32_t>((1.0f - y / depth) * screenshot.extent.height * 0.5f);
    return Pixel(screenshot, pixel_x, pixel_y);
}

bool NearColor(Color actual, Color expected, int tolerance = 4) {
    for (std::size_t index = 0; index < 3; ++index)
        if (std::abs(static_cast<int>(actual[index]) - expected[index]) > tolerance) return false;
    return true;
}

std::uint64_t ChangedPixels(const Screenshot& left, const Screenshot& right) {
    if (left.extent.width != right.extent.width || left.extent.height != right.extent.height)
        throw std::invalid_argument("comparison extent mismatch");
    std::uint64_t changed = 0;
    for (std::size_t offset = 0; offset < left.pixels.size(); offset += 4)
        if (!std::equal(left.pixels.begin() + offset, left.pixels.begin() + offset + 3, right.pixels.begin() + offset)) ++changed;
    return changed;
}

Value ColorDocument(Color color) { return Value{std::vector<std::uint32_t>(color.begin(), color.end())}; }

class Session {
public:
    Session(Renderer& renderer, Window& window, Report& report) : renderer_(renderer), window_(window), report_(report) {}

    FrameResult Render(std::string_view name, const std::vector<MeshDraw>& draws, bool screenshot = true,
                       std::optional<SelectionBox> selection = {}, std::optional<CameraParameters> camera = {},
                       std::optional<std::uint64_t> retry_frame_id = {}) {
        FrameInput frame;
        frame.frame_id = retry_frame_id ? *retry_frame_id : NextId();
        frame.camera.vertical_fov_radians = std::numbers::pi_v<float> / 2.0f;
        frame.camera.near_plane = 0.1f;
        frame.camera.far_plane = 100.0f;
        if (camera) frame.camera = *camera;
        frame.draws = draws;
        frame.selection = selection;
        frame.request_screenshot = screenshot;
        const auto result = renderer_.Render(frame);
        Value row;
        row["name"] = name;
        row["frame_id"] = result.frame_id;
        row["status"] = result.status == FrameStatus::Presented ? "Presented" : "Skipped";
        row["skip_reason"] = static_cast<std::uint32_t>(result.skip_reason);
        row["cpu_prepare_ns"] = result.statistics.cpu_prepare.count();
        row["cpu_submit_ns"] = result.statistics.cpu_submit.count();
        row["present_wait_ns"] = result.statistics.present_wait.count();
        row["upload_bytes"] = result.statistics.upload_bytes;
        row["draw_calls"] = result.statistics.draw_calls;
        row["drawn_vertices"] = result.statistics.drawn_vertices;
        row["requested_screenshot"] = screenshot;
        row["has_screenshot"] = result.screenshot.has_value();
        report_.value["frames"].push_back(std::move(row));
        report_.Check(result.frame_id == frame.frame_id, std::string(name) + ": exact frame association");
        if (result.status == FrameStatus::Presented) {
            submitted_.insert(frame.frame_id);
            if (result.statistics.draw_calls > 0) drawn_.insert(frame.frame_id);
            report_.Check(result.skip_reason == FrameSkipReason::None, std::string(name) + ": presented has no skip reason");
            report_.Check(result.screenshot.has_value() == screenshot, std::string(name) + ": screenshot request association");
            if (result.screenshot) {
                const auto& capture = *result.screenshot;
                const auto info = renderer_.GetInfo();
                report_.Check(capture.extent.width == info.framebuffer_extent.width &&
                    capture.extent.height == info.framebuffer_extent.height &&
                    capture.pixels.size() == static_cast<std::size_t>(capture.extent.width) * capture.extent.height * 4,
                    std::string(name) + ": tight actual-pixel readback");
                report_.Capture(name, capture);
            }
        } else {
            report_.Check(result.skip_reason != FrameSkipReason::None && !result.screenshot,
                std::string(name) + ": skipped frame has explicit reason and no fake screenshot");
        }
        Poll();
        return result;
    }

    void Poll() {
        for (const auto& sample : renderer_.PollGpuSamples()) {
            Value row;
            row["frame_id"] = sample.frame_id;
            row["valid"] = sample.valid;
            row["missing_reason"] = static_cast<std::uint32_t>(sample.missing_reason);
            row["gpu_draw_time_ns"] = sample.gpu_draw_time ? Value{sample.gpu_draw_time->count()} : Value{};
            report_.value["gpu_samples"].push_back(std::move(row));
            report_.Check(submitted_.contains(sample.frame_id), "GPU sample maps to an actually presented sparse frame ID");
            report_.Check(received_.insert(sample.frame_id).second, "GPU sample frame ID is delivered once");
            report_.Check(sample.valid && sample.missing_reason == GpuSampleMissingReason::None &&
                sample.gpu_draw_time && sample.gpu_draw_time->count() >= 0,
                "ready GPU sample has an actual nonnegative duration; missing samples are never replaced by zero");
            if (drawn_.contains(sample.frame_id) && sample.gpu_draw_time->count() > 0) ++positive_draw_samples_;
        }
    }

    void Drain() {
        const auto deadline = Clock::now() + std::chrono::seconds{5};
        while (received_.size() != submitted_.size() && Clock::now() < deadline) {
            window_.PollInt();
            Poll();
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        report_.value["presented_sparse_frame_ids"] = Value::Sequence{};
        for (const auto id : submitted_) report_.value["presented_sparse_frame_ids"].push_back(id);
        report_.Check(submitted_ == received_, "bounded drain associates every measured GPU sample with exact submitted IDs");
        report_.value["positive_nonempty_draw_samples"] = positive_draw_samples_;
        report_.Check(positive_draw_samples_ > 0,
            "at least one nonempty native draw has a positive measured GPU interval; samples are not fixed zero");
    }

    std::uint64_t NextId() {
        static constexpr std::array<std::uint64_t, 4> increments{7, 29, 1009, 100003};
        serial_ += increments[step_++ % increments.size()];
        return serial_;
    }

private:
    Renderer& renderer_;
    Window& window_;
    Report& report_;
    std::uint64_t serial_ = 0;
    std::size_t step_ = 0;
    std::set<std::uint64_t> submitted_;
    std::set<std::uint64_t> received_;
    std::set<std::uint64_t> drawn_;
    std::uint64_t positive_draw_samples_ = 0;
};

const Screenshot& Capture(const FrameResult& result) {
    if (result.status != FrameStatus::Presented || !result.screenshot)
        throw std::runtime_error("normal scene was not presented with an actual screenshot");
    return *result.screenshot;
}

template<class Operation>
void ExpectInvalidHandle(Report& report, std::string_view name, Operation operation) {
    bool rejected = false;
    try { operation(); }
    catch (const RenderError& error) {
        report.value["expected_errors"].push_back(ErrorDocument(error));
        rejected = error.Category() == RenderErrorCategory::InvalidHandle;
    }
    report.Check(rejected, name);
}

template<class Operation>
void ExpectUnsupported(Report& report, std::string_view name, Operation operation) {
    bool rejected = false;
    try { operation(); }
    catch (const RenderError& error) {
        report.value["expected_errors"].push_back(ErrorDocument(error));
        rejected = error.Category() == RenderErrorCategory::Unsupported;
    }
    report.Check(rejected, name);
}

void RunNormal(const Options& options, Window& window, Report& report) {
    RendererConfig config;
    config.backend = Backend::D3D12;
    config.requested_samples = options.samples;
    config.vsync = options.vsync;
    config.shader_asset_root = options.shaders;
    Renderer renderer{window, config};
    const auto initial = renderer.GetInfo();
    report.value["initial_backend_info"] = InfoDocument(initial);
    report.Check(initial.backend == Backend::D3D12 && !initial.device_name.empty() && !initial.device_identity.empty(),
        "actual D3D12 device identity is reported");
    report.Check(initial.device_name == options.expected_device_name && !initial.driver_version.empty() && initial.validation_enabled,
        "verification runs on the expected RTX device with actual driver identity and enabled Debug Layer");
    report.Check(initial.actual_samples == options.samples, "actual sample count equals request without silent fallback");
    report.Check(initial.actual_present_mode == (options.vsync ? PresentMode::Fifo : PresentMode::Immediate),
        "initial actual present mode matches requested VSync");
    report.Check(initial.capabilities.texture_arrays && initial.capabilities.selection_boxes &&
        initial.capabilities.screenshots && initial.capabilities.gpu_timestamps, "native implemented capabilities are available");
    const auto initial_drawable = Drawable(window);
    report.Check(initial.framebuffer_extent.width == initial_drawable.width && initial.framebuffer_extent.height == initial_drawable.height,
        "initial framebuffer reports drawable pixels rather than logical request");

    Session session{renderer, window, report};
    auto texture_data = Fixture();
    const auto texture = renderer.CreateTextureArray(texture_data);
    auto quad_data = Quad(0);
    const auto quad = renderer.CreateMesh(quad_data);
    quad_data.vertices.clear();
    std::fill(texture_data.pixels.begin(), texture_data.pixels.end(), 0);
    const auto blank = session.Render("00-empty", {});
    const auto initial_frame = session.Render("01-owned-fixture", {Draw(quad, texture)});
    const auto& fixture = Capture(initial_frame);
    Value pixel_observation;
    pixel_observation["top_left"] = ColorDocument(WorldPixel(fixture, -0.5f, 0.5f));
    pixel_observation["bottom_left"] = ColorDocument(WorldPixel(fixture, -0.5f, -0.5f));
    pixel_observation["bottom_right"] = ColorDocument(WorldPixel(fixture, 0.5f, -0.5f));
    report.Check(NearColor(WorldPixel(fixture, -0.5f, 0.5f), Red) &&
        NearColor(WorldPixel(fixture, -0.5f, -0.5f), Green) && NearColor(WorldPixel(fixture, 0.5f, -0.5f), Yellow),
        "owned CPU upload, top-left texture orientation, and both front-facing triangles", std::move(pixel_observation));
    const auto repeated = session.Render("02-persistent", {Draw(quad, texture)});
    report.Check(ChangedPixels(fixture, Capture(repeated)) == 0 && repeated.statistics.upload_bytes == 0,
        "persistent mesh and texture render identically without reupload");

    const auto blue_quad = renderer.CreateMesh(Quad(1));
    const auto red_quad = renderer.CreateMesh(Quad(0, false, glm::vec2{0.25f, 0.25f}));
    const auto depth_front_first = session.Render("03-depth-front-first", {Draw(red_quad, texture), Draw(blue_quad, texture, -4)});
    const auto depth_back_first = session.Render("04-depth-back-first", {Draw(blue_quad, texture, -4), Draw(red_quad, texture)});
    report.Check(NearColor(WorldPixel(Capture(depth_front_first), 0, 0), Red) &&
        ChangedPixels(Capture(depth_front_first), Capture(depth_back_first)) == 0,
        "RH negative-Z depth occlusion is independent of draw order");
    const auto reverse_quad = renderer.CreateMesh(Quad(0, true));
    const auto culled = session.Render("05-reverse-winding", {Draw(reverse_quad, texture)});
    report.Check(ChangedPixels(Capture(blank), Capture(culled)) == 0,
        "both reversed back-facing triangles are culled in actual pixels");
    const auto transparent_quad = renderer.CreateMesh(Quad(0, false, glm::vec2{0.75f, 0.25f}));
    const auto alpha = session.Render("06-alpha-discard", {Draw(transparent_quad, texture), Draw(blue_quad, texture, -4)});
    report.Check(NearColor(WorldPixel(Capture(alpha), 0, 0), Blue), "alpha-discard foreground does not hide the later background");
    const auto alpha_reverse = session.Render("06b-alpha-reverse-order", {Draw(blue_quad, texture, -4), Draw(transparent_quad, texture)});
    report.Check(ChangedPixels(Capture(alpha), Capture(alpha_reverse)) == 0,
        "alpha discard preserves background identically in both draw orders");
    const auto repeated_uv_quad = renderer.CreateMesh(Quad(0, false, glm::vec2{-0.75f, 2.25f}));
    const auto repeat_uv = session.Render("06c-repeat-addressing", {Draw(repeated_uv_quad, texture)});
    report.Check(ChangedPixels(Capture(depth_front_first), Capture(repeat_uv)) == 0,
        "negative and greater-than-one UV repeat to the same red texel as UV 0.25,0.25");
    const auto point_quad = renderer.CreateMesh(Quad(0, false, glm::vec2{0.1f, 0.1f}));
    const auto point_sample = session.Render("06d-point-filtering", {Draw(point_quad, texture)});
    report.Check(NearColor(WorldPixel(Capture(point_sample), 0, 0), Red),
        "fixed non-boundary UV uses point filtering rather than blending wrapped neighboring texels");

    auto solid_layers = Fixture();
    for (std::size_t offset = 0; offset < solid_layers.pixels.size(); offset += 4) {
        const auto& color = offset < solid_layers.pixels.size() / 2 ? Red : Blue;
        std::copy(color.begin(), color.end(), solid_layers.pixels.begin() + offset);
    }
    const auto solid_texture = renderer.CreateTextureArray(solid_layers);
    MeshData mixed_triangle;
    mixed_triangle.vertices = {
        {{-1, -1, 0}, {0.25f, 0.25f, 0}, 1.0f},
        {{1, -1, 0}, {0.25f, 0.25f, 0}, 1.0f},
        {{0, 1, 0}, {0.25f, 0.25f, 1}, 1.0f}
    };
    const auto mixed_mesh = renderer.CreateMesh(mixed_triangle);
    const auto mixed_frame = session.Render("06e-interpolated-array-layer", {Draw(mixed_mesh, solid_texture)});
    report.Check(NearColor(WorldPixel(Capture(mixed_frame), 0, -0.5f), Red) &&
        NearColor(WorldPixel(Capture(mixed_frame), 0, 0.5f), Blue),
        "integer vertex layers interpolate to fractional array selection inside one triangle, not flat material IDs");
    for (auto& vertex : mixed_triangle.vertices) vertex.tex_coord.z = 0;
    const auto edge_mesh = renderer.CreateMesh(mixed_triangle);
    const auto edge_frame = session.Render("06f-physical-msaa-edge", {Draw(edge_mesh, solid_texture)});
    const auto clear_color = Pixel(Capture(blank), 0, 0);
    std::uint64_t intermediate_coverage = 0;
    for (std::uint32_t y = 0; y < Capture(edge_frame).extent.height; ++y)
        for (std::uint32_t x = 0; x < Capture(edge_frame).extent.width; ++x) {
            const auto color = Pixel(Capture(edge_frame), x, y);
            if (!NearColor(color, Red) && !NearColor(color, clear_color)) ++intermediate_coverage;
        }
    report.value["physical_msaa_edge_probe"]["actual_samples"] = initial.actual_samples;
    report.value["physical_msaa_edge_probe"]["intermediate_coverage_pixels"] = intermediate_coverage;
    report.Check(options.samples == 4 ? intermediate_coverage >= 10 : intermediate_coverage == 0,
        "diagonal triangle has physical multisample resolve coverage only when samples=4", Value{intermediate_coverage});
    const auto near_clip = session.Render("07-near-plane", {Draw(red_quad, texture, -0.05f, 0.01f)});
    const auto behind = session.Render("08-behind-camera", {Draw(red_quad, texture, 3)});
    report.Check(ChangedPixels(Capture(blank), Capture(near_clip)) == 0 && ChangedPixels(Capture(blank), Capture(behind)) == 0,
        "RH_ZO near clipping and behind-camera rejection agree with projection");
    const SelectionBox selection{{-0.8f, -0.8f, -3.5f}, {0.8f, 0.8f, -2.5f}};
    const auto selected = session.Render("09-selection", {Draw(red_quad, texture)}, true, selection);
    const auto selection_pixels = ChangedPixels(Capture(depth_front_first), Capture(selected));
    report.Check(selection_pixels >= 20, "selection wireframe changes actual scene pixels", Value{selection_pixels});

    auto updated_data = Quad(1);
    renderer.UpdateMesh(quad, updated_data);
    updated_data.vertices.clear();
    const auto updated = session.Render("10-updated", {Draw(quad, texture)});
    report.Check(NearColor(WorldPixel(Capture(updated), 0, 0), Blue),
        "mesh update owns replacement bytes and draws the newly uploaded persistent geometry");
    report.value["upload_statistics_scope"] = "persistent mesh/texture uploads occur in Create/Update; frame upload_bytes counts only work staged by that frame, including selection vertices";
    const auto updated_repeat = session.Render("11-updated-persistent", {Draw(quad, texture)});
    report.Check(updated_repeat.statistics.upload_bytes == 0, "updated mesh becomes persistent after first upload");
    renderer.UpdateMesh(quad, MeshData{});
    const auto emptied = session.Render("12-empty-update", {Draw(quad, texture)});
    report.Check(ChangedPixels(Capture(blank), Capture(emptied)) == 0 && emptied.statistics.draw_calls == 0,
        "empty mesh update retires old geometry and creates no draw call");
    renderer.DestroyMesh(quad);
    ExpectInvalidHandle(report, "destroyed mesh handle cannot be destroyed twice", [&] { renderer.DestroyMesh(quad); });
    ExpectInvalidHandle(report, "destroyed mesh handle cannot be updated", [&] { renderer.UpdateMesh(quad, Quad(0)); });
    renderer.DestroyTextureArray(texture);
    ExpectInvalidHandle(report, "destroyed texture handle cannot be destroyed twice", [&] { renderer.DestroyTextureArray(texture); });
    FrameInput invalid_frame;
    invalid_frame.frame_id = session.NextId();
    const std::array stale_draws{Draw(red_quad, texture)};
    invalid_frame.draws = stale_draws;
    ExpectInvalidHandle(report, "stale texture binding is rejected before submission", [&] { renderer.Render(invalid_frame); });

    const auto replacement_texture = renderer.CreateTextureArray(Fixture());
    const auto replacement_mesh = renderer.CreateMesh(Quad(0));
    const auto replacement = session.Render("13-generation-reuse", {Draw(replacement_mesh, replacement_texture)});
    report.Check(NearColor(WorldPixel(Capture(replacement), -0.5f, 0.5f), Red), "new resource generations remain usable after stale-handle rejection");

    const auto gray_linear = renderer.CreateTextureArray(GrayFixture(ColorSpace::Linear));
    const auto gray_srgb = renderer.CreateTextureArray(GrayFixture(ColorSpace::SRGB));
    const auto linear_frame = session.Render("14-linear-gray", {Draw(red_quad, gray_linear)});
    const auto srgb_frame = session.Render("15-srgb-gray", {Draw(red_quad, gray_srgb)});
    const auto linear_color = WorldPixel(Capture(linear_frame), 0, 0);
    const auto srgb_color = WorldPixel(Capture(srgb_frame), 0, 0);
    Value color_spaces;
    color_spaces["linear_pixel"] = ColorDocument(linear_color);
    color_spaces["srgb_decoded_pixel"] = ColorDocument(srgb_color);
    report.Check(NearColor(linear_color, Color{64, 64, 64, 255}) && NearColor(srgb_color, Color{13, 13, 13, 255}),
        "Linear SRV preserves values and SRGB SRV decodes to linear UNORM output", std::move(color_spaces));

    const auto image = Assets::DecodeImage(options.atlas, false);
    auto atlas_data = NormalizeAtlas(image);
    report.value["atlas_normalization"]["source_width"] = image.width;
    report.value["atlas_normalization"]["source_height"] = image.height;
    report.value["atlas_normalization"]["source_channels"] = image.channels;
    report.value["atlas_normalization"]["tile_extent"] = 64;
    report.value["atlas_normalization"]["layers"] = atlas_data.layers;
    report.value["atlas_normalization"]["layout"] = "bottom tile row first, then ascending columns; each layer has original top-left tightly packed RGBA8 rows; no within-tile vertical flip";
    std::array<Color, 4> atlas_expected{};
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 4> atlas_coordinates{{{16,16}, {48,16}, {16,48}, {48,48}}};
    for (std::size_t index = 0; index < atlas_expected.size(); ++index) {
        const auto [x, y] = atlas_coordinates[index];
        const auto offset = (static_cast<std::size_t>(y) * atlas_data.width + x) * 4;
        std::copy_n(atlas_data.pixels.begin() + offset, 4, atlas_expected[index].begin());
    }
    const auto atlas_texture = renderer.CreateTextureArray(atlas_data);
    atlas_data.pixels.clear();
    const auto atlas_frame = session.Render("16-real-atlas", {Draw(replacement_mesh, atlas_texture)});
    report.Check(ChangedPixels(Capture(blank), Capture(atlas_frame)) >= 100,
        "real Assets::DecodeImage atlas normalization uploads and draws visible native pixels");
    const std::array<glm::vec2, 4> atlas_world{{{-0.5f,0.5f}, {0.5f,0.5f}, {-0.5f,-0.5f}, {0.5f,-0.5f}}};
    Value atlas_comparison = Value::Sequence{};
    bool atlas_matches = true;
    for (std::size_t index = 0; index < atlas_expected.size(); ++index) {
        const auto actual = WorldPixel(Capture(atlas_frame), atlas_world[index].x, atlas_world[index].y);
        Value point;
        point["layer"] = 0;
        point["texel_x"] = atlas_coordinates[index].first;
        point["texel_y"] = atlas_coordinates[index].second;
        point["normalized_cpu_rgba"] = ColorDocument(atlas_expected[index]);
        point["actual_gpu_rgba"] = ColorDocument(actual);
        atlas_comparison.push_back(std::move(point));
        atlas_matches = atlas_matches && atlas_expected[index][3] >= 3 && NearColor(actual, atlas_expected[index]);
    }
    report.Check(atlas_matches, "real atlas layer zero matches bottom-first tile and top-left CPU texels in native screenshot",
        std::move(atlas_comparison));

    window.SetSize(803, 557);
    const auto resized_drawable = Drawable(window);
    renderer.Resize(resized_drawable);
    const auto resized = session.Render("17-resized", {Draw(replacement_mesh, replacement_texture)});
    report.Check(Capture(resized).extent.width == resized_drawable.width && Capture(resized).extent.height == resized_drawable.height,
        "non-aligned resize and readback use actual drawable pixels and strip GPU row pitch");

    renderer.Resize({0, resized_drawable.height});
    report.value["explicit_zero_info"] = InfoDocument(renderer.GetInfo());
    const auto explicit_zero = session.Render("18-explicit-zero", {Draw(replacement_mesh, replacement_texture)}, false);
    report.Check(explicit_zero.status == FrameStatus::Skipped && explicit_zero.skip_reason == FrameSkipReason::ZeroExtent &&
        explicit_zero.statistics.draw_calls == 0 && explicit_zero.statistics.upload_bytes == 0,
        "explicit zero extent is suspended without submit, upload, present, or screenshot");
    renderer.Resize(resized_drawable);
    const auto restored = session.Render("19-explicit-restored", {Draw(replacement_mesh, replacement_texture)});
    report.Check(restored.status == FrameStatus::Presented, "explicit zero extent can restore actual native output");

    const HWND hwnd = Platform::NativeBridge::GetHandle(window);
    ShowWindow(hwnd, SW_MINIMIZE);
    const auto minimize_deadline = Clock::now() + std::chrono::seconds{3};
    while (!window.Minimized() && Clock::now() < minimize_deadline) { window.WaitEvents(0.02); }
    report.Check(window.Minimized() && IsIconic(hwnd), "real Win32 minimization is observed independently of explicit zero Resize");
    renderer.Resize(Drawable(window));
    const auto minimized = session.Render("20-real-minimized", {}, false);
    report.Check(minimized.status == FrameStatus::Skipped && minimized.skip_reason == FrameSkipReason::ZeroExtent,
        "real minimized drawable is skipped with explicit zero-extent status");
    const auto hold_begin = Clock::now();
    const auto hold_deadline = hold_begin + std::chrono::seconds{10};
    bool continuously_minimized = true;
    std::uint64_t hold_poll_count = 0;
    while (Clock::now() < hold_deadline) {
        const auto remaining = std::chrono::duration<double>(hold_deadline - Clock::now()).count();
        window.WaitEvents(std::clamp(remaining, 0.0, 0.02));
        ++hold_poll_count;
        if (!window.Minimized() || !IsIconic(hwnd)) {
            continuously_minimized = false;
            break;
        }
    }
    const auto actual_hold_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - hold_begin).count();
    report.value["actual_minimized_hold_ms"] = actual_hold_ms;
    report.value["minimized_hold_poll_count"] = hold_poll_count;
    report.value["minimum_requested_minimized_hold_ms"] = 10000;
    report.Check(continuously_minimized && window.Minimized() && IsIconic(hwnd) && actual_hold_ms >= 10000,
        "real minimization remains observed continuously for at least ten seconds without additional Render calls");
    ShowWindow(hwnd, SW_RESTORE);
    const auto restore_deadline = Clock::now() + std::chrono::seconds{3};
    while (window.Minimized() && Clock::now() < restore_deadline) { window.WaitEvents(0.02); }
    report.Check(!window.Minimized() && !IsIconic(hwnd), "real Win32 restore is observed");
    renderer.Resize(Drawable(window));
    const auto real_restored = session.Render("21-real-restored", {Draw(replacement_mesh, replacement_texture)});
    report.Check(real_restored.status == FrameStatus::Presented, "real minimization restores native render output");

    renderer.SetVSync(true);
    report.value["vsync_enabled_info"] = InfoDocument(renderer.GetInfo());
    report.Check(renderer.GetInfo().actual_present_mode == PresentMode::Fifo, "SetVSync true reports actual FIFO present mode");
    session.Render("22-vsync-on", {Draw(replacement_mesh, replacement_texture)});
    renderer.SetVSync(false);
    report.value["vsync_disabled_info"] = InfoDocument(renderer.GetInfo());
    report.Check(renderer.GetInfo().actual_present_mode == PresentMode::Immediate, "SetVSync false reports actual immediate present mode");
    session.Render("23-vsync-off", {Draw(replacement_mesh, replacement_texture)});

    const auto retired_texture = renderer.CreateTextureArray(Fixture());
    const auto retired_mesh = renderer.CreateMesh(Quad(1));
    session.Render("24-retire-after-async-submit", {Draw(retired_mesh, retired_texture)}, false);
    renderer.DestroyMesh(retired_mesh);
    renderer.DestroyTextureArray(retired_texture);
    report.value["resource_retirement_probe"] = "no-screenshot submit followed by immediate logical destroy; actual GPU overlap is not forced or claimed";
    const auto after_retirement = session.Render("25-after-logical-retirement", {Draw(replacement_mesh, replacement_texture)});
    report.Check(NearColor(WorldPixel(Capture(after_retirement), -0.5f, 0.5f), Red),
        "logical destroy after no-screenshot submission does not corrupt later native work");

    CameraParameters large_direction;
    large_direction.vertical_fov_radians = std::numbers::pi_v<float> / 2.0f;
    large_direction.far_plane = 100.0f;
    const auto maximum_float = std::numeric_limits<float>::max();
    large_direction.forward = {0, 0, -maximum_float};
    large_direction.up = {0, maximum_float, 0};
    const auto stable_direction = session.Render("26-finite-large-direction", {Draw(replacement_mesh, replacement_texture)},
        true, {}, large_direction);
    report.Check(NearColor(WorldPixel(Capture(stable_direction), -0.5f, 0.5f), Red),
        "FLT_MAX finite camera directions normalize stably and preserve native projection pixels");
    auto large_position = large_direction;
    large_position.position = {0, 0, 1.0e20f};
    large_position.forward = {0, 0, -1};
    large_position.up = {0, 1, 0};
    auto distant_quad_data = Quad(0);
    for (auto& vertex : distant_quad_data.vertices) vertex.pos_coord.z = -3;
    const auto distant_quad = renderer.CreateMesh(distant_quad_data);
    auto distant_draw = Draw(distant_quad, replacement_texture, 0);
    distant_draw.object_transform[3][2] = large_position.position.z;
    const auto stable_position = session.Render("27-finite-large-position", {distant_draw}, true, {}, large_position);
    session.Render("28-current-empty", {});
    report.Check(ChangedPixels(Capture(stable_position), Capture(after_retirement)) == 0 &&
        NearColor(WorldPixel(Capture(stable_position), -0.5f, 0.5f), Red),
        "large finite eye and equal model translation cancel before projection; raw integer z=-3 preserves near-plane depth and pixels");
    report.value["large_position_draw_probe"]["eye_z"] = large_position.position.z;
    report.value["large_position_draw_probe"]["model_translation_z"] = distant_draw.object_transform[3][2];
    report.value["large_position_draw_probe"]["raw_integer_vertex_z"] = -3;

    const std::array extreme_draws{Draw(replacement_mesh, replacement_texture)};
    FrameInput oversized_selection;
    oversized_selection.frame_id = session.NextId();
    oversized_selection.draws = extreme_draws;
    oversized_selection.camera.vertical_fov_radians = std::numbers::pi_v<float> / 2.0f;
    oversized_selection.selection = SelectionBox{glm::vec3{-maximum_float}, glm::vec3{maximum_float}};
    ExpectUnsupported(report, "finite selection expansion outside GPU float range is rejected before Execute",
        [&] { renderer.Render(oversized_selection); });
    const auto selection_retry = session.Render("29-selection-same-id-retry", {Draw(replacement_mesh, replacement_texture)},
        true, {}, {}, oversized_selection.frame_id);
    report.Check(selection_retry.status == FrameStatus::Presented && selection_retry.frame_id == oversized_selection.frame_id &&
        NearColor(WorldPixel(Capture(selection_retry), -0.5f, 0.5f), Red),
        "Unsupported selection is nonfatal and does not consume the frame ID before a corrected native retry");

    FrameInput tiny_fov;
    tiny_fov.frame_id = session.NextId();
    tiny_fov.draws = extreme_draws;
    tiny_fov.camera.vertical_fov_radians = std::numeric_limits<float>::denorm_min();
    ExpectUnsupported(report, "positive finite FOV producing unrepresentable GPU matrix is rejected before Execute",
        [&] { renderer.Render(tiny_fov); });
    const auto fov_retry = session.Render("30-fov-same-id-retry", {Draw(replacement_mesh, replacement_texture)},
        true, {}, {}, tiny_fov.frame_id);
    report.Check(fov_retry.status == FrameStatus::Presented && fov_retry.frame_id == tiny_fov.frame_id &&
        NearColor(WorldPixel(Capture(fov_retry), -0.5f, 0.5f), Red),
        "Unsupported matrix is nonfatal and preserves frame ID for a corrected native retry");

    report.value["async_timestamp_screenshot_requested"] = false;
    for (int index = 0; index < 5; ++index) session.Render("async-" + std::to_string(index), {Draw(replacement_mesh, replacement_texture)}, false);
    session.Drain();
    report.value["final_backend_info"] = InfoDocument(renderer.GetInfo());
    const auto shutdown_begin = Clock::now();
    renderer.Shutdown();
    renderer.Shutdown();
    const auto shutdown_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - shutdown_begin).count();
    report.value["repeat_shutdown_ms"] = shutdown_ms;
    report.Check(shutdown_ms < 5000, "normal bounded shutdown drains owned native work and is repeatable");
}

void RunConstructionFailure(const Options& options, Window& window, Report& report) {
    RendererConfig config;
    config.backend = Backend::D3D12;
    config.shader_asset_root = options.shaders;
    config.requested_samples = options.samples;
    config.vsync = options.vsync;
    DWORD handles_before = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &handles_before)) throw std::runtime_error("GetProcessHandleCount(before) failed");
    bool rejected = false;
    const auto begin = Clock::now();
    try { Renderer renderer{window, config}; }
    catch (const RenderError& error) {
        report.value["expected_constructor_error"] = ErrorDocument(error);
        rejected = error.Category() == RenderErrorCategory::AssetUnavailable ||
            error.Category() == RenderErrorCategory::InitializationFailure;
    }
    DWORD handles_after = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &handles_after)) throw std::runtime_error("GetProcessHandleCount(after) failed");
    report.value["process_handles_before"] = handles_before;
    report.value["process_handles_after"] = handles_after;
    report.value["handle_observation_is_leak_proof"] = false;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - begin).count();
    report.value["failed_constructor_ms"] = elapsed;
    report.Check(rejected, "missing or malformed shader is a real typed constructor failure");
    report.Check(elapsed < 5000, "failed shader initialization and partial-state cleanup are bounded");
    report.Check(IsWindow(Platform::NativeBridge::GetHandle(window)) && !window.ShouldClose(),
        "failed renderer construction does not destroy its borrowed Native Window");
    // A second failure on the same window checks cleanup without a production fault hook.
    bool second_rejected = false;
    try { Renderer renderer{window, config}; }
    catch (const RenderError& error) {
        report.value["repeat_constructor_error"] = ErrorDocument(error);
        second_rejected = error.Category() == RenderErrorCategory::AssetUnavailable ||
            error.Category() == RenderErrorCategory::InitializationFailure;
    }
    report.Check(second_rejected, "repeated partial initialization leaves borrowed window usable");
    config.shader_asset_root = options.recovery_shaders;
    Renderer recovered{window, config};
    report.value["recovery_backend_info"] = InfoDocument(recovered.GetInfo());
    report.Check(recovered.GetInfo().device_name == options.expected_device_name && !recovered.GetInfo().driver_version.empty() &&
        recovered.GetInfo().validation_enabled, "constructor recovery uses the expected actual RTX device and enabled Debug Layer");
    Session session{recovered, window, report};
    const auto texture = recovered.CreateTextureArray(Fixture());
    const auto mesh = recovered.CreateMesh(Quad(0));
    const auto recovery = session.Render("constructor-recovery", {Draw(mesh, texture)});
    report.Check(NearColor(WorldPixel(Capture(recovery), -0.5f, 0.5f), Red),
        "good shaders initialize and draw on the same borrowed window after repeated constructor failures");
    session.Drain();
    recovered.Shutdown();
    recovered.Shutdown();
}
} // namespace

int main(int argc, char** argv) {
    std::unique_ptr<Report> report;
    try {
        const auto options = ParseOptions(argc, argv);
        report = std::make_unique<Report>(options);
        report->Save();
        NativeWindow window;
        if (options.mode == "normal") RunNormal(options, window.Get(), *report);
        else RunConstructionFailure(options, window.Get(), *report);
        report->value["success"] = true;
        report->Save();
        return 0;
    } catch (const RenderError& error) {
        if (report) { report->value["failure"] = ErrorDocument(error); report->Save(); }
        std::cerr << "RenderError: " << error.what() << " operation=" << error.Operation()
                  << " native=" << error.NativeCode() << " diagnostic=" << error.Diagnostic() << '\n';
    } catch (const std::exception& error) {
        if (report) { report->value["failure"]["message"] = error.what(); report->Save(); }
        std::cerr << "Failure: " << error.what() << '\n';
    }
    return 1;
}
