#include "game_renderer.h"
#include "startup_options.h"
#include <native_bridge.h>
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/renderer/renderer.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/world/benchmark_workload.h>
#include <symocraft/world/test_scene.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <thread>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using Value = Data::Value;
using Clock = std::chrono::steady_clock;
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Mark(const char* name) { std::cout << "BATCH2_MARK_" << name << std::endl; }
void JsonString(std::ostream& stream, const std::string& text) {
    constexpr char hex[] = "0123456789abcdef";
    stream << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') stream << '\\' << c;
        else if (c < 32) stream << "\\u00" << hex[c >> 4] << hex[c & 15];
        else stream << c;
    }
    stream << '"';
}
void Json(std::ostream& stream, const Value& value) {
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) stream << "null";
        else if constexpr (std::is_same_v<T, bool>) stream << (item ? "true" : "false");
        else if constexpr (std::is_same_v<T, std::string>) JsonString(stream, item);
        else if constexpr (std::is_same_v<T, Value::Mapping>) {
            stream << '{'; bool first = true;
            for (const auto& [key, child] : item) {
                if (!first) stream << ','; first = false; JsonString(stream, key); stream << ':'; Json(stream, child);
            }
            stream << '}';
        } else if constexpr (std::is_same_v<T, Value::Sequence>) {
            stream << '['; bool first = true;
            for (const auto& child : item) { if (!first) stream << ','; first = false; Json(stream, child); }
            stream << ']';
        } else stream << item;
    }, value.storage());
}
std::uint64_t Digest(const Screenshot& image) {
    std::uint64_t result = 14695981039346656037ull;
    for (auto value : image.pixels) { result ^= value; result *= 1099511628211ull; }
    return result;
}
void Save(Value& report, const std::filesystem::path& output, const std::string& name, const Screenshot& image) {
    Check(image.extent.width && image.extent.height && image.pixels.size() == size_t(image.extent.width)*image.extent.height*4, "invalid screenshot storage");
    Assets::WritePng(output/(name+".png"),static_cast<int>(image.extent.width),static_cast<int>(image.extent.height),4,image.pixels);
    Value row; row["name"] = name; row["width"] = image.extent.width; row["height"] = image.extent.height; row["rgba_fnv1a64"] = Digest(image);
    report["captures"].push_back(row);
}
using Color = std::array<int,3>;
Color Pixel(const Screenshot& image, unsigned x, unsigned y) {
    const auto i = (size_t(y)*image.extent.width+x)*4;
    return {image.pixels.at(i),image.pixels.at(i+1),image.pixels.at(i+2)};
}
bool Near(Color a, Color b) { for (int c=0;c<3;++c) if (std::abs(a[c]-b[c])>2) return false; return true; }
size_t Count(const Screenshot& image, Color color) {
    size_t count=0; for (unsigned y=0;y<image.extent.height;++y) for (unsigned x=0;x<image.extent.width;++x) count += Near(Pixel(image,x,y),color); return count;
}
void Nonblank(const Screenshot& image) { Check(Count(image,{135,206,235})+5000 < size_t(image.extent.width)*image.extent.height,"complete world screenshot is blank"); }
struct OwnedWindow {
    std::unique_ptr<Window> value;
    OwnedWindow(Backend backend, int width, int height) {
        Window::Init(); value.reset(Window::Create("SymoCraft production draw/window probe",width,height,false,true,
            backend == Backend::Vulkan ? WindowMode::Vulkan : WindowMode::Native));
    }
    void Close() { value->Destroy(); value.reset(); Window::Free(); }
    ~OwnedWindow() { try { if (value) Close(); } catch (...) {} }
};
Extent2D Await(Window& window, HWND hwnd, bool minimized) {
    const auto deadline=Clock::now()+std::chrono::seconds(3);
    do {
        window.PollInt(); Check(!window.ShouldClose(),"window closed during automatic verification");
        const bool iconic=IsIconic(hwnd)!=0;
        if (minimized && window.Minimized() && iconic) return {};
        if (!minimized && !window.Minimized() && !iconic && window.width>0 && window.height>0) {
            const auto previous=SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            Check(previous!=nullptr,"cannot query physical client pixels"); RECT rect{};
            const bool ok=GetClientRect(hwnd,&rect)!=0; const bool restored=SetThreadDpiAwarenessContext(previous)!=nullptr;
            Check(ok && restored,"client query or DPI context restore failed");
            if (rect.right-rect.left==window.width && rect.bottom-rect.top==window.height) return {unsigned(window.width),unsigned(window.height)};
        }
        window.WaitEvents(.01);
    } while (Clock::now()<deadline);
    throw std::runtime_error("native/SDL drawable state did not converge");
}
MeshData Quad(int z, int layer, float repeat=1, int radius=1, bool reverse=false) {
    const glm::ivec3 p[]{{-radius,-radius,z},{radius,-radius,z},{radius,radius,z},{-radius,-radius,z},{radius,radius,z},{-radius,radius,z}};
    const glm::vec2 uv[]{{0,repeat},{repeat,repeat},{repeat,0},{0,repeat},{repeat,0},{0,0}};
    MeshData result;
    for (size_t i=0;i<6;++i) result.vertices.push_back({p[i],{uv[i],float(layer)},0});
    if (reverse) for (size_t i=0;i<6;i+=3) std::swap(result.vertices[i],result.vertices[i+1]);
    return result;
}
TextureArrayData SolidAtlas() {
    TextureArrayData data; data.width=data.height=1; data.layers=4;
    data.pixels={230,20,40,0, 20,40,230,255, 20,220,50,255, 230,20,40,1}; return data;
}
void Drawing(Backend backend, unsigned samples, const std::filesystem::path& output, Value& report) {
    OwnedWindow owner(backend,640,480); auto& window=*owner.value;
    RendererConfig config; config.backend=backend; config.requested_samples=samples; config.vsync=false; config.enable_validation=true;
    config.shader_asset_root=Assets::Resolve(backend==Backend::Vulkan ? "shaders/vulkan" : "shaders/d3d12");
    Renderer renderer(window,config); const auto info=renderer.GetInfo();
    Check(info.backend==backend && info.actual_samples==samples && info.validation_enabled && info.device_name.find("RTX 5070 Ti")!=std::string::npos,"actual backend/device/samples/validation mismatch");
    Check(info.actual_present_mode==PresentMode::Immediate,"VSync-off is not actual Immediate");
    report["device"]=info.device_name; report["identity"]=info.device_identity; report["api"]=info.api_version; report["driver"]=info.driver_version;
    std::vector<MeshHandle> meshes; std::vector<TextureArrayHandle> textures;
    auto make=[&](MeshData data) { const auto h=renderer.CreateMesh(data); meshes.push_back(h); data.vertices.clear(); return h; };
    auto texture=[&](TextureArrayData data) { const auto h=renderer.CreateTextureArray(data); textures.push_back(h); data.pixels.clear(); return h; };
    const auto atlas=texture(SolidAtlas()); const auto back=make(Quad(0,1,1,2)), front=make(Quad(1,0));
    FrameInput input; input.camera.position={0,0,5}; std::uint64_t id=1;
    std::vector<MeshDraw> draws;
    auto render=[&](const std::string& name) {
        window.PollInt(); input.frame_id=id++; input.draws=draws; input.request_screenshot=true;
        const auto result=renderer.Render(input); Check(result.status==FrameStatus::Presented && result.screenshot.has_value(),"drawing did not present/capture");
        Save(report,output,name,*result.screenshot); return *result.screenshot;
    };
    const auto clear=render("clear"); draws={{back,atlas}}; const auto baseline=render("background");
    Check(Near(Pixel(baseline,320,240),{20,40,230}),"background layer mismatch");
    draws={{front,atlas},{back,atlas}}; Check(render("discard-front-first").pixels==baseline.pixels,"discard wrote depth before background");
    std::reverse(draws.begin(),draws.end()); Check(render("discard-back-first").pixels==baseline.pixels,"discard changed background after depth");
    renderer.UpdateMesh(front,Quad(1,3)); Check(render("discard-alpha-one").pixels==baseline.pixels,"alpha below cutoff was not discarded");
    renderer.UpdateMesh(front,Quad(1,2)); auto opaque=render("opaque-depth");
    Check(Near(Pixel(opaque,320,240),{20,220,50}),"near opaque mesh lost depth");
    std::reverse(draws.begin(),draws.end()); Check(render("opaque-reversed-order").pixels==opaque.pixels,"opaque depth depends on draw order");
    renderer.UpdateMesh(front,{}); Check(render("empty-update").pixels==baseline.pixels,"empty update retained old geometry");
    renderer.UpdateMesh(front,Quad(1,2)); Check(render("empty-restored").pixels==opaque.pixels,"nonempty update did not recover");
    const auto reversed=make(Quad(0,2,1,1,true)); draws={{reversed,atlas}};
    Check(render("backface-culled").pixels==clear.pixels,"back-face winding was not culled");
    TextureArrayData tiled; tiled.width=tiled.height=2; tiled.layers=2;
    tiled.pixels={230,20,40,255,20,220,50,255,20,40,230,255,230,200,20,255,128,128,128,255,128,128,128,255,128,128,128,255,128,128,128,255};
    const auto linear=texture(tiled); tiled.color_space=ColorSpace::SRGB; const auto srgb=texture(tiled);
    const auto uv=make(Quad(0,0,2)); draws={{uv,linear}}; auto image=render("uv-repeat-linear");
    auto patch=[&](const Screenshot& capture, float x, float y, Color expected) {
        const float focal=1/std::tan(input.camera.vertical_fov_radians*.5f);
        const unsigned px=unsigned((1+x*focal/(5*capture.extent.AspectRatio()))*capture.extent.width*.5);
        const unsigned py=unsigned((1-y*focal/5)*capture.extent.height*.5);
        Check(Near(Pixel(capture,px,py),expected),"UV repeat/direction/color patch mismatch");
    };
    for (float x : {-.75f,.25f}) for (float y : {.75f,-.25f}) patch(image,x,y,{230,20,40});
    patch(image,-.25f,.75f,{20,220,50}); patch(image,-.75f,.25f,{20,40,230}); patch(image,-.25f,.25f,{230,200,20});
    draws={{uv,srgb}}; image=render("uv-repeat-srgb");
    auto decode=[](int channel) { const double x=channel/255.; return int(std::lround((x<=.04045 ? x/12.92 : std::pow((x+.055)/1.055,2.4))*255)); };
    patch(image,-.75f,.75f,{decode(230),decode(20),decode(40)});
    renderer.UpdateMesh(uv,Quad(0,1)); image=render("second-layer-srgb"); patch(image,0,0,{55,55,55});
    draws={{back,atlas}}; input.selection=SelectionBox{{-1,-1,0},{1,1,.2f}};
    image=render("selection-visible"); size_t selection_pixels=0;
    // A one-pixel line can cover only a subset of the MSAA samples.
    for (unsigned covered=1;covered<=samples;++covered) {
        Color resolved{};
        for (int c=0;c<3;++c) resolved[c]=int(std::lround((Color{20,40,230}[c]*double(samples-covered)+Color{220,20,60}[c]*double(covered))/samples));
        selection_pixels+=Count(image,resolved);
    }
    Check(selection_pixels>30,"visible selection missing at any valid MSAA coverage"); report["selection_pixels"]=selection_pixels;
    const auto cover=make(Quad(1,2,1,2)); draws={{cover,atlas}}; input.selection.reset(); const auto cover_image=render("selection-cover");
    input.selection=SelectionBox{{-1,-1,0},{1,1,.2f}};
    Check(render("selection-occluded").pixels==cover_image.pixels,"selection incorrectly ignores opaque depth"); input.selection.reset();
    auto mixed=Quad(0,0); mixed.vertices[1].tex_coord.z=1; mixed.vertices[2].tex_coord.z=2;
    const auto mixed_handle=make(mixed); draws={{mixed_handle,atlas}}; render("mixed-layer-triangle");
    auto invalid=Quad(0,99); const auto invalid_handle=make(invalid);
    input.draws=std::span<const MeshDraw>{}; input.frame_id=id; input.request_screenshot=false;
    const std::array<MeshDraw,1> bad_draw{{{invalid_handle,atlas}}}; input.draws=bad_draw;
    Mark("INVALID_FRAME_BEGIN"); bool rejected=false;
    try { (void)renderer.Render(input); } catch (const RenderError& e) { rejected=e.Category()==RenderErrorCategory::InvalidInput; }
    Mark("INVALID_FRAME_END"); Check(rejected,"out-of-range texture layer accepted");
    draws={{back,atlas}}; render("invalid-frame-recovered");
    renderer.SetVSync(true); Check(renderer.GetInfo().actual_present_mode==PresentMode::Fifo,"VSync-on did not report Fifo"); render("fifo");
    renderer.SetVSync(false); Check(renderer.GetInfo().actual_present_mode==PresentMode::Immediate,"VSync-off silently downgraded"); render("immediate");
    const auto hwnd=Platform::NativeBridge::GetHandle(window);
    Mark("WSI_RESIZE_BEGIN");
    Check(SetWindowPos(hwnd,nullptr,0,0,800,600,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE)!=0,"native resize failed");
    input.frame_id=id++; input.request_screenshot=false; input.draws=draws;
    report["vulkan_wsi_race_applicable"]=backend==Backend::Vulkan;
    const auto raced=renderer.Render(input); report["wsi_race_skipped"]=raced.status==FrameStatus::Skipped;
    if (backend==Backend::D3D12) Check(raced.skip_reason==FrameSkipReason::Occluded,"native extent drift was not skipped before D3D12 submission");
    if (backend==Backend::Vulkan) {
        const auto previous=renderer.GetInfo(); bool pending=false; Mark("PENDING_RESIZE_BEGIN");
        try { renderer.Resize({641,481}); }
        catch (const RenderError& e) { pending=e.Category()==RenderErrorCategory::InvalidState && e.Diagnostic()=="surface_extent_pending"; }
        const auto preserved=renderer.GetInfo(); Mark("PENDING_RESIZE_END");
        Check(pending && preserved.framebuffer_extent.width==previous.framebuffer_extent.width &&
            preserved.framebuffer_extent.height==previous.framebuffer_extent.height && preserved.actual_present_mode==previous.actual_present_mode,
            "pending surface resize published requested rather than accepted native info");
        report["pending_resize_preserved_info"]=true;
    }
    const auto extent=Await(window,hwnd,false); renderer.Resize(extent);
    render("wsi-recovered"); Mark("WSI_RESIZE_END");
    for (auto h : meshes) renderer.DestroyMesh(h);
    for (auto h : textures) renderer.DestroyTextureArray(h);
    renderer.Shutdown(); renderer.Shutdown();
    config.requested_samples=32; bool unsupported=false;
    try { Renderer excessive(window,config); excessive.Shutdown(); }
    catch (const RenderError& e) { unsupported=e.Category()==RenderErrorCategory::Unsupported; }
    Check(unsupported,"32x MSAA was not rejected as Unsupported on this device");
    report["unsupported_32x_rejected"]=true; report["invalid_layer_rejected_before_native_frame"]=true;
    report["checks"]="alpha cutoff/order/depth; winding; empty recovery; UV repeat/orientation; linear/sRGB/layers; selection depth; mixed layer; VSync; native WSI resize";
    owner.Close();
}
using Revisions=std::map<std::pair<int,int>,World::Revision>;
Revisions RevisionsOf(const World::VoxelWorld& world) {
    Revisions result; world.VisitMeshes([&](const World::WorldMeshRecord& item) { result[{item.identity.chunk.x,item.identity.chunk.y}]=item.revision; }); return result;
}
void WorldWindow(Backend backend, unsigned samples, const std::filesystem::path& output, Value& report) {
    OwnedWindow owner(backend,1920,1080); auto& window=*owner.value; const auto hwnd=Platform::NativeBridge::GetHandle(window);
    StartupOptions options; options.renderer=backend==Backend::Vulkan ? "vulkan" : "d3d12"; options.requested_samples=samples; options.enable_validation=true;
    Application::Detail::GameRenderer renderer(window,options,false);
    const auto config_bytes=Assets::ReadBytes(Assets::Resolve("configs/blockFormats.yaml"));
    auto definition=World::BlockDefinition::FromConfig(std::string(config_bytes.begin(),config_bytes.end()));
    const auto layers=renderer.LoadAtlas(Assets::Resolve("textures/texture_atlas.png")); definition.ValidateTextureLayers(layers);
    Generation::Settings settings; settings.seed=424242;
    auto world=World::VoxelWorld::Create(std::move(definition),settings); TestScene::Install(*world);
    Check(world->ChunkCount()==441,"world workload was reduced");
    report["chunks"]=world->ChunkCount(); report["initial_digest"]=world->Digest();
    report["initial_rebuilt_meshes"]=world->RebuildDirtyMeshes(); const auto first=renderer.Synchronize(*world);
    report["initial_upload_calls"]=first.calls; report["initial_upload_bytes"]=first.bytes;
    ECS::Registry registry; registry.RegisterComponent<Transform>("Transform"); Camera camera(registry,1920,1080);
    auto pose=[&](const TestScene::Pose& value) { camera.SetCameraPos(value.position); camera.SetYaw(value.yaw); camera.SetPitch(value.pitch); TransformSystem::Update(registry); };
    std::uint64_t id=1; unsigned capture_skips=0;
    auto capture=[&](const std::string& name) {
        const auto deadline=Clock::now()+std::chrono::seconds(3);
        do {
            window.PollInt(); const auto frame=renderer.Render(id++,camera,{},nullptr,true).result;
            if (frame.status==FrameStatus::Skipped) {
                Check(frame.skip_reason==FrameSkipReason::ZeroExtent || frame.skip_reason==FrameSkipReason::Occluded,"unexpected capture skip");
                report["capture_skips"]=++capture_skips; window.WaitEvents(.01); continue;
            }
            Check(frame.screenshot.has_value() && frame.statistics.draw_calls==361,"complete world did not render all 361 accepted meshes");
            const auto& image=*frame.screenshot;
            Check(image.extent.width==unsigned(window.width) && image.extent.height==unsigned(window.height),"capture disagrees with real drawable pixels");
            Nonblank(image); Save(report,output,name,image); return image;
        } while (Clock::now()<deadline);
        throw std::runtime_error("world capture did not resume within three seconds");
    };
    std::map<std::string,Screenshot> original;
    for (const auto& p : TestScene::Checkpoints()) { pose(p); original[std::string(p.name)]=capture(std::string("initial-")+std::string(p.name)); }
    auto static_upload=renderer.Synchronize(*world); Check(static_upload.calls==0 && static_upload.bytes==0,"unchanged world reuploaded geometry");
    pose(TestScene::Checkpoint("four-chunk")); const auto reference=original.at("four-chunk");
    camera.SetCameraPos(camera.GetCameraPos()+glm::vec3(1.5f,0,0)); camera.SetYaw(camera.GetYaw()+15); TransformSystem::Update(registry);
    Check(capture("camera-move-turn").pixels!=reference.pixels,"camera movement/turn did not change projection");
    camera.Scroll(10); Check(camera.GetFov()==35,"camera zoom did not reach production getter"); capture("camera-zoom"); camera.Scroll(-10);
    Check(renderer.Synchronize(*world).calls==0,"camera change reuploaded world geometry");
    for (unsigned i=0;i<32;++i) {
        const auto edit=Benchmark::CycleEdit(i); Check(world->QueryBlock(edit.position).block.block_id==edit.before,"edit precondition differs");
        const auto before=RevisionsOf(*world);
        Check(world->TryEdit({World::EditOperation::Set,edit.position,edit.after}).status==World::EditStatus::Changed,"edit was not applied");
        const auto rebuilt=world->RebuildDirtyMeshes(); const auto after=RevisionsOf(*world); size_t changed=0;
        for (const auto& [key,revision] : after) { const auto previous=before.find(key); if (previous==before.end() || previous->second!=revision) ++changed; }
        const auto upload=renderer.Synchronize(*world);
        Check(rebuilt==changed && upload.calls==changed && changed>0,"cache updated other than changed published chunks");
        Value row; row["index"]=i; row["x"]=edit.position.x; row["y"]=edit.position.y; row["z"]=edit.position.z; row["changed_records"]=changed; row["upload_calls"]=upload.calls; row["upload_bytes"]=upload.bytes;
        report["edits"].push_back(row);
        (void)renderer.Render(id++,camera,{});
        if (i==15 || i==31) for (const auto& p : TestScene::Checkpoints()) {
            pose(p); const auto image=capture(std::string(i==15 ? "edited-" : "restored-")+std::string(p.name));
            if (i==31) Check(image.pixels==original.at(std::string(p.name)).pixels,"reverse edits did not restore exact same-view pixels");
        }
    }
    Check(world->Digest()==report["initial_digest"].as<std::string>(),"reverse edits did not restore world digest");
    pose(TestScene::Checkpoint("four-chunk"));
    for (const auto& size : {std::pair{960,540},std::pair{800,600},std::pair{1280,720},std::pair{1920,1080}}) {
        window.SetSize(size.first,size.second); const auto pixels=Await(window,hwnd,false); renderer.Resize();
        const auto image=capture("resize-"+std::to_string(size.first)+"x"+std::to_string(size.second));
        Check(image.extent.width==pixels.width && image.extent.height==pixels.height,"resize did not use real pixel extent");
        Check(Platform::NativeBridge::GetHandle(window)==hwnd,"Window was hot recreated");
    }
    ShowWindow(hwnd,SW_MINIMIZE); Await(window,hwnd,true); renderer.Resize();
    report["minimized_sdl_width"]=window.width; report["minimized_sdl_height"]=window.height;
    Mark("PAUSE_BEGIN"); const auto start=Clock::now(); unsigned steps=0;
    while (Clock::now()-start<std::chrono::seconds(10)) {
        const auto wake=Clock::now()+std::chrono::milliseconds(100);
        window.PollInt(); Check(window.Minimized() && IsIconic(hwnd),"actual minimize state lost");
        const auto frame=renderer.Render(id++,camera,{}).result;
        Check(frame.status==FrameStatus::Skipped && frame.skip_reason==FrameSkipReason::ZeroExtent && frame.statistics.draw_calls==0 && frame.statistics.upload_bytes==0 && !frame.screenshot,"minimized frame was not suspended");
        ++steps; window.WaitEvents(.09); std::this_thread::sleep_until(wake);
    }
    report["pause_seconds"]=std::chrono::duration<double>(Clock::now()-start).count(); report["pause_steps"]=steps;
    Check(steps>=50 && steps<=150,"pause used a busy loop"); Mark("PAUSE_END");
    ShowWindow(hwnd,SW_RESTORE); Await(window,hwnd,false); renderer.Resize();
    Check(capture("window-restored").pixels==reference.pixels,"same-view world/ratio did not recover after minimize");
    report["focused_before_input_check"]=window.Focused();
    (void)SetForegroundWindow(hwnd);
    const auto focus_deadline=Clock::now()+std::chrono::seconds(2);
    while ((!window.Focused() || GetForegroundWindow()!=hwnd) && Clock::now()<focus_deadline) { window.PollInt(); window.WaitEvents(.01); }
    Check(window.Focused() && GetForegroundWindow()==hwnd,"owned restored window did not gain real focus");
    window.ResetInput();
    const auto key_data=LPARAM(1)|(LPARAM(MapVirtualKeyW('W',MAPVK_VK_TO_VSC))<<16);
    Check(PostMessageW(hwnd,WM_KEYDOWN,'W',key_data)!=0 && PostMessageW(hwnd,WM_KEYUP,'W',key_data|(LPARAM(3)<<30))!=0,"cannot post owned-window key pair");
    const auto input_deadline=Clock::now()+std::chrono::seconds(2); bool key=false;
    while (!key && Clock::now()<input_deadline) { window.CaptureInput(); key=window.Input().Down(Key::W); if (!key) window.WaitEvents(.01); }
    Check(key,"native-window synthetic key did not resume input");
    camera.SetCameraPos(camera.GetCameraPos()+camera.GetCameraFront()*.5f); TransformSystem::Update(registry);
    Check(capture("restored-input-camera").pixels!=reference.pixels,"post-restore camera input had no visible effect");
    window.CaptureInput(); Check(!window.Input().Down(Key::W),"synthetic released key remained stuck");
    report["synthetic_native_key_after_restore"]=true; report["no_performance_session"]=true;
    renderer.ReleaseWorld(world->Identity()); renderer.Shutdown(); renderer.Shutdown(); owner.Close();
}
}
int main(int argc, char** argv) {
    if (argc!=5) return 64;
    Value report; report["schema"]="symocraft.renderer-draw-window.v1"; report["success"]=false;
    const std::filesystem::path output=std::filesystem::absolute(argv[3]);
    if (std::filesystem::exists(output)) return 64;
    std::filesystem::create_directories(output);
    int code=1;
    try {
        const std::string backend_name=argv[1], mode=argv[2]; const unsigned samples=std::stoul(argv[4]);
        Check((backend_name=="vulkan" || backend_name=="d3d12") && (mode=="drawing" || mode=="world-window") && (samples==1 || samples==4),"invalid backend/mode/samples");
        const auto backend=backend_name=="vulkan" ? Backend::Vulkan : Backend::D3D12;
        report["backend"]=backend_name; report["mode"]=mode; report["samples"]=samples;
        if (mode=="drawing") Drawing(backend,samples,output,report); else WorldWindow(backend,samples,output,report);
        report["success"]=true; code=0;
    } catch (const std::exception& error) { report["error"]=error.what(); std::cerr << "[probe] " << error.what() << '\n'; }
    std::ofstream file(output/"report.json"); Json(file,report); file << '\n';
    if (!file) return 2;
    return code;
}
