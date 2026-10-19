#include "diagnostic_access.h"
#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/renderer.h>
#include <symocraft/telemetry/document_io.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <type_traits>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Value = Data::Value;
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void Json(std::ostream& out, const Value& value) {
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) out << "null";
        else if constexpr (std::is_same_v<T, bool>) out << (item ? "true" : "false");
        else if constexpr (std::is_same_v<T, std::string>) {
            out << '"'; constexpr char hex[] = "0123456789abcdef";
            for (unsigned char c : item) {
                if (c == '"' || c == '\\') out << '\\' << c;
                else if (c < 32) out << "\\u00" << hex[c >> 4] << hex[c & 15]; else out << c;
            }
            out << '"';
        } else if constexpr (std::is_same_v<T, Value::Mapping>) {
            out << '{'; bool first = true;
            for (const auto& [key, child] : item) {
                if (!first) out << ','; first = false; Json(out, Value(key)); out << ':'; Json(out, child);
            }
            out << '}';
        } else if constexpr (std::is_same_v<T, Value::Sequence>) {
            out << '['; bool first = true;
            for (const auto& child : item) { if (!first) out << ','; first = false; Json(out, child); }
            out << ']';
        } else out << item;
    }, value.storage());
}
MeshData Quad() {
    MeshData mesh;
    const glm::ivec3 p[]{{-1,-1,0},{1,-1,0},{1,1,0},{-1,-1,0},{1,1,0},{-1,1,0}};
    const glm::vec2 uv[]{{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
    for (size_t i=0;i<6;++i) mesh.vertices.push_back({p[i],{uv[i],0},0});
    return mesh;
}
}

int main(int argc, char** argv) {
    std::unique_ptr<Platform::Runtime> runtime;
    std::unique_ptr<Window> window;
    try {
        Require(argc == 4, "usage: probe mode shader-root new-output-directory");
        const std::string mode = argv[1];
        const bool capability = mode == "missing-extension" || mode == "missing-feature";
        Require(capability || mode == "acquire-out-of-date" || mode == "acquire-suboptimal" ||
                mode == "present-suboptimal" || mode == "present-out-of-date", "unknown mode");
        const auto output = std::filesystem::absolute(argv[3]);
        Require(!std::filesystem::exists(output), "output must be new");
        std::filesystem::create_directories(output);
        runtime = std::make_unique<Platform::Runtime>();
        window = Window::Create(*runtime,"Vulkan capability and WSI diagnostic",640,480,false,true,WindowMode::Vulkan);
        auto d = std::make_shared<VulkanDiagnosticAccess>();
        d->wait_timeout = std::chrono::milliseconds(3000);
        d->hide_maintenance_extension = mode == "missing-extension";
        d->hide_maintenance_feature = mode == "missing-feature";
        Value report;
        report["schema"] = "symocraft.vulkan-wsi.v1"; report["mode"] = mode;
        report["public_facade_and_production_backend"] = true;
        report["controlled_injection"] = true; report["performance_acceptance"] = false;
        {
            ScopedVulkanDiagnostics binding(d);
            RendererConfig config; config.backend = Backend::Vulkan;
            config.shader_asset_root = std::filesystem::absolute(argv[2]);
            config.enable_validation = true; config.vsync = false; config.requested_samples = 1;
            if (capability) {
                bool refused = false;
                try { Renderer renderer(*window, config); }
                catch (const RenderError& error) {
                    refused = error.Category() == RenderErrorCategory::Unsupported && error.BackendName() == "Vulkan" &&
                              error.Operation() == "SelectDevice";
                    report["error_operation"] = error.Operation(); report["error_message"] = error.what();
                }
                Require(refused && d->device_creations == 0, "capability did not reject before CreateDevice");
                Require(d->hide_maintenance_extension ? d->extension_rejections > 0 : d->feature_rejections > 0,
                        "requested capability rejection was not exercised");
                report["unsupported_before_device"] = true;
            } else {
                Renderer renderer(*window,config);
                const auto info = renderer.GetInfo();
                Require(info.backend == Backend::Vulkan && info.device_name.find("RTX 5070 Ti") != std::string::npos &&
                        info.validation_enabled && info.actual_samples == 1 &&
                        info.actual_present_mode == PresentMode::Immediate, "actual device/configuration mismatch");
                report["device"] = info.device_name; report["identity"] = info.device_identity;
                report["driver"] = info.driver_version; report["api"] = info.api_version;
                report["validation_enabled"] = info.validation_enabled; report["samples"] = info.actual_samples;
                report["present_mode"] = "Immediate";
                auto mesh = renderer.CreateMesh(Quad());
                TextureArrayData atlas; atlas.width = atlas.height = 2; atlas.layers = 1;
                atlas.pixels = {230,20,40,255,20,220,50,255,20,40,230,255,230,200,20,255};
                auto texture = renderer.CreateTextureArray(atlas);
                std::array<MeshDraw,1> draws{{{mesh,texture,glm::mat4(1)}}};
                FrameInput frame; frame.draws = draws; frame.camera.position = {0,0,4};
                std::set<uint64_t> ids, delivered;
                const auto render = [&] {
                    window->PollInt(); frame.frame_id++; frame.request_screenshot = true;
                    ids.insert(frame.frame_id); return renderer.Render(frame);
                };
                auto baseline = render();
                Require(baseline.status == FrameStatus::Presented && baseline.screenshot, "baseline did not present");
                Require(baseline.statistics.draw_calls == 1 &&
                        std::count(baseline.screenshot->pixels.begin(), baseline.screenshot->pixels.end(), uint8_t{230}) > 100,
                        "baseline texture did not draw");
                Assets::WritePng(output/"baseline.png",640,480,4,baseline.screenshot->pixels);
                report["events"] = Value::Sequence{};
                const auto fence_status = reinterpret_cast<PFN_vkGetFenceStatus>(d->get_device_proc(d->device,"vkGetFenceStatus"));
                Require(fence_status != nullptr, "native fence status unavailable");
                for (unsigned repeat=0;repeat<3;++repeat) {
                    const auto submits = d->successful_submits, presents = d->present_calls;
                    const auto acquires = d->acquire_calls, generation = d->swapchain_creations;
                    if (mode == "acquire-out-of-date") d->next_acquire = VK_ERROR_OUT_OF_DATE_KHR;
                    if (mode == "acquire-suboptimal") d->next_acquire = VK_SUBOPTIMAL_KHR;
                    if (mode == "present-suboptimal") d->next_present = VK_SUBOPTIMAL_KHR;
                    if (mode == "present-out-of-date") d->next_present = VK_ERROR_OUT_OF_DATE_KHR;
                    auto result = render();
                    const bool no_acquire = mode == "acquire-out-of-date";
                    const bool skipped = no_acquire || mode == "present-out-of-date";
                    Require(skipped ? (result.status == FrameStatus::Skipped && !result.screenshot &&
                            result.skip_reason == FrameSkipReason::Occluded) : result.status == FrameStatus::Presented,
                            "wrong WSI branch result");
                    Require(d->successful_submits == submits + !no_acquire &&
                            d->present_calls == presents + !no_acquire &&
                            d->acquire_calls == acquires + !no_acquire, "unbalanced acquire/submit/present");
                    Require(fence_status(d->device,d->last_submit) == VK_SUCCESS, "submit fence left reset without signal");
                    Value event;
                    event["actual_acquire"] = no_acquire ? Value{} : Value(static_cast<int>(d->last_actual_acquire));
                    event["effective_acquire"] = static_cast<int>(d->last_effective_acquire);
                    event["actual_present"] = no_acquire ? Value{} : Value(static_cast<int>(d->last_actual_present));
                    event["effective_present"] = no_acquire ? Value{} : Value(static_cast<int>(d->last_effective_present));
                    event["native_acquire_calls"] = d->acquire_calls - acquires;
                    event["native_submit_calls"] = d->successful_submits - submits;
                    event["native_present_calls"] = d->present_calls - presents;
                    event["submit_fence_signaled"] = true;
                    auto recovered = render();
                    Require(recovered.status == FrameStatus::Presented && recovered.screenshot &&
                            recovered.screenshot->pixels == baseline.screenshot->pixels, "recovery image differs");
                    Require(d->swapchain_creations == generation + 1, "next Render did not rebuild exactly once");
                    event["rebuilt_once"] = true; event["recovered_pixels_equal"] = true;
                    report["events"].push_back(std::move(event));
                    Assets::WritePng(output/("recovered-"+std::to_string(repeat)+".png"),640,480,4,recovered.screenshot->pixels);
                }
                for (const auto& sample : renderer.PollGpuSamples()) {
                    Require(ids.contains(sample.frame_id) && delivered.insert(sample.frame_id).second, "invalid/duplicate sample ID");
                    Require(sample.valid ? sample.gpu_draw_time.has_value() :
                        (mode == "acquire-out-of-date" && sample.missing_reason == GpuSampleMissingReason::Discarded),
                        "unexpected WSI sample reason");
                }
                Require(ids == delivered, "WSI sample identities lost");
                report["sample_ids_complete"] = true;
                renderer.DestroyMesh(mesh); renderer.DestroyTextureArray(texture);
                renderer.Shutdown(); renderer.Shutdown();
            }
        }
        Require(d->native_destroyed && d->loader_released && !d->quarantined && d->live_bytes == 0,
                "native cleanup incomplete");
        Require(d->warnings == 0 && d->errors == 0, "validation diagnostics reported");
        report["extension_rejections"] = d->extension_rejections; report["feature_rejections"] = d->feature_rejections;
        report["device_creations"] = d->device_creations; report["swapchain_creations"] = d->swapchain_creations;
        report["native_retired"] = true; report["validation_warnings"] = d->warnings; report["validation_errors"] = d->errors;
        report["success"] = true;
        std::ofstream out(output/"report.json"); Json(out, report); out << '\n'; Require(out.good(),"cannot write report");
        window->Destroy(); window.reset(); runtime->Shutdown(); runtime.reset();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        if (window) { window->Destroy(); window.reset(); }
        return 1;
    }
}

