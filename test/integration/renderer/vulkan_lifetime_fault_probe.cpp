#include "diagnostic_access.h"
#include "game_entry.h"
#include "application.h"
#include <Windows.h>
#include <native_bridge.h>
#include <symocraft/platform/window.h>
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/renderer/renderer.h>
#include <symocraft/telemetry/document_io.h>
#include <algorithm>
#include <array>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <utility>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Value = Data::Value;
using Clock = std::chrono::steady_clock;
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void Check(VkResult result) { Require(result==VK_SUCCESS,"owned native diagnostic operation failed"); }
template<class T> T Proc(VulkanDiagnosticAccess& d, const char* name) {
    auto p=reinterpret_cast<T>(d.get_device_proc(d.device,name)); Require(p!=nullptr,"missing native diagnostic procedure"); return p;
}
void Json(std::ostream& out, const Value& value) {
    std::visit([&](const auto& item) {
        using T=std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T,std::monostate>) out<<"null";
        else if constexpr (std::is_same_v<T,bool>) out<<(item?"true":"false");
        else if constexpr (std::is_same_v<T,std::string>) {
            out<<'"'; constexpr char hex[]="0123456789abcdef";
            for (unsigned char c:item) {
                if (c=='"'||c=='\\') out<<'\\'<<c;
                else if (c<32) out<<"\\u00"<<hex[c>>4]<<hex[c&15]; else out<<c;
            } out<<'"';
        } else if constexpr (std::is_same_v<T,Value::Mapping>) {
            out<<'{'; bool first=true;
            for (const auto& [key,child]:item) { if(!first)out<<','; first=false; Json(out,Value(key)); out<<':'; Json(out,child); } out<<'}';
        } else if constexpr (std::is_same_v<T,Value::Sequence>) {
            out<<'['; bool first=true; for(const auto& child:item){if(!first)out<<',';first=false;Json(out,child);} out<<']';
        } else out<<item;
    },value.storage());
}
struct Gate {
    VulkanDiagnosticAccess& d;
    VkSemaphore event{};
    std::mutex mutex;
    std::condition_variable condition;
    bool released{}, watchdog_fired{};
    std::thread watchdog;
    explicit Gate(VulkanDiagnosticAccess& access):d(access) {
        Require(d.enable_timeline_gate,"isolated timeline gate feature was not requested");
        VkSemaphoreTypeCreateInfo type{VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO}; type.semaphoreType=VK_SEMAPHORE_TYPE_TIMELINE;
        VkSemaphoreCreateInfo create{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}; create.pNext=&type;
        Check(Proc<PFN_vkCreateSemaphore>(d,"vkCreateSemaphore")(d.device,&create,nullptr,&event));
        d.next_gate=event;
        // Fail open before an OS watchdog interval, without submitting a busy shader.
        watchdog=std::thread([this] {
            std::unique_lock lock(mutex);
            if(!condition.wait_for(lock,std::chrono::milliseconds(900),[&]{return released;})) {
                watchdog_fired=true; Signal();
            }
        });
    }
    void Signal() {
        VkSemaphoreSignalInfo signal{VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO}; signal.semaphore=event; signal.value=1;
        Check(Proc<PFN_vkSignalSemaphore>(d,"vkSignalSemaphore")(d.device,&signal));
    }
    void Release() {
        {std::lock_guard lock(mutex); if(!released) { if(!watchdog_fired)Signal(); released=true; }}
        condition.notify_all(); if(watchdog.joinable())watchdog.join();
    }
    void Close() {
        Release();
        if(d.last_submit) Check(Proc<PFN_vkWaitForFences>(d,"vkWaitForFences")(d.device,1,&d.last_submit,VK_TRUE,2'000'000'000));
        if(d.deferred_present_pending&&!d.quarantined)d.finish_deferred_present();
        Proc<PFN_vkDestroySemaphore>(d,"vkDestroySemaphore")(d.device,event,nullptr); event={};
        Require(!watchdog_fired,"diagnostic gate watchdog fired; real pending assertion invalid");
    }
    ~Gate() { try { if(event)Close(); } catch(...) {} }
    VkResult Status() { return Proc<PFN_vkGetFenceStatus>(d,"vkGetFenceStatus")(d.device,d.last_submit); }
    std::uint64_t Counter() { std::uint64_t value{}; Check(Proc<PFN_vkGetSemaphoreCounterValue>(d,"vkGetSemaphoreCounterValue")(d.device,event,&value)); return value; }
};
struct OwnedWindow {
    std::unique_ptr<Window> window;
    OwnedWindow() { Window::Init(); window.reset(Window::Create("Vulkan production lifetime/fault probe",640,480,false,true,WindowMode::Vulkan)); }
    ~OwnedWindow() { try { window->Destroy(); window.reset(); Window::Free(); } catch(...) {} }
};
MeshData Quad(unsigned layer=0,unsigned copies=1) {
    MeshData data; const glm::ivec3 p[]{{-1,-1,0},{1,-1,0},{1,1,0},{-1,-1,0},{1,1,0},{-1,1,0}};
    for(unsigned i=0;i<copies;++i) for(auto v:p) data.vertices.push_back({v,{.5f,.5f,float(layer)},0}); return data;
}
TextureArrayData Texture() {
    TextureArrayData data; data.width=data.height=1; data.layers=2; data.pixels={230,20,40,255,20,220,50,255}; return data;
}
template<class F> RenderError Expect(F action, RenderErrorCategory category) {
    try { action(); } catch(const RenderError& e) { Require(e.Category()==category,"unexpected renderer error category"); return e; }
    throw std::runtime_error("expected renderer rejection missing");
}
void Stats(Value& report,const VulkanDiagnosticAccess& d) {
    report["buffers_created"]=d.buffers_created; report["buffers_released"]=d.buffers_released;
    report["images_created"]=d.images_created; report["images_released"]=d.images_released;
    report["live_bytes"]=d.live_bytes; report["peak_bytes"]=d.peak_bytes;
    report["submit_attempts"]=d.submit_attempts; report["successful_submits"]=d.successful_submits;
    report["present_calls"]=d.present_calls; report["validation_warnings"]=d.warnings; report["validation_errors"]=d.errors;
    report["quarantined"]=d.quarantined; report["native_destroyed"]=d.native_destroyed; report["loader_released"]=d.loader_released;
    report["isolated_timeline_gate_feature"]=d.enable_timeline_gate;
    report["last_actual_present_result"]=int(d.last_actual_present);
    report["native_wait_calls"]=d.wait_calls; report["native_query_calls"]=d.query_calls;
}
void Failed(Renderer& r,VulkanDiagnosticAccess& d,const RenderError& first,Value& report) {
    report["first_category"]=int(first.Category()); report["first_operation"]=first.Operation(); report["first_native_code"]=first.NativeCode();
    const auto attempts=d.submit_attempts, presents=d.present_calls;
    Expect([&]{r.CreateMesh(Quad());},RenderErrorCategory::InvalidState);
    Expect([&]{(void)r.Render(FrameInput{});},RenderErrorCategory::InvalidState);
    Expect([&]{r.Resize({800,600});},RenderErrorCategory::InvalidState);
    Expect([&]{(void)r.PollGpuSamples();},RenderErrorCategory::InvalidState);
    (void)r.GetInfo();
    Expect([&]{r.Shutdown();},RenderErrorCategory::Timeout);
    Expect([&]{r.Shutdown();},RenderErrorCategory::Timeout);
    Require(attempts==d.submit_attempts&&presents==d.present_calls,"failed renderer continued submission");
    report["failed_rejections"]=4; report["repeated_shutdown_not_success"]=true; report["no_further_submit_or_present"]=true;
}
void Facade(const std::string& mode,const std::filesystem::path& output,Value& report) {
    OwnedWindow owner; auto d=std::make_shared<VulkanDiagnosticAccess>(); ScopedVulkanDiagnostics binding(d);
    d->enable_timeline_gate=mode=="lifetime"||mode=="texture-timeout"||mode=="frame-timeout"||mode=="async-gated";
    d->disable_timestamps=mode=="async-unsupported";
    RendererConfig config; config.backend=Backend::Vulkan; config.enable_validation=true; config.vsync=false;
    config.shader_asset_root=Assets::Resolve("shaders/vulkan");
    if(mode=="pipeline") {
        d->next_pipeline=VK_ERROR_OUT_OF_DEVICE_MEMORY;
        const auto e=Expect([&]{Renderer partial(*owner.window,config);},RenderErrorCategory::InitializationFailure);
        report["first_operation"]=e.Operation(); report["first_native_code"]=e.NativeCode();
        Require(d->native_destroyed&&d->loader_released&&!d->quarantined&&d->live_bytes==0,"partial pipeline owner not fully reclaimed");
        Stats(report,*d); return;
    }
    auto renderer=std::make_unique<Renderer>(*owner.window,config); auto& r=*renderer;
    const auto info=r.GetInfo(); Require(info.device_name.find("RTX 5070 Ti")!=std::string::npos&&info.validation_enabled,"wrong actual device/validation");
    report["device"]=info.device_name; report["identity"]=info.device_identity; report["driver"]=info.driver_version;
    auto mesh=r.CreateMesh(Quad()); auto texture=r.CreateTextureArray(Texture());
    std::uint64_t id=1; std::vector<MeshDraw> draws{{mesh,texture}};
    auto frame=[&](bool screenshot=false) {
        owner.window->PollInt(); FrameInput input; input.frame_id=id++; input.camera.position={0,0,5}; input.draws=draws; input.request_screenshot=screenshot;
        auto result=r.Render(input);
        Require(result.status==FrameStatus::Presented || (d->deferred_present_pending&&result.status==FrameStatus::Skipped&&result.skip_reason==FrameSkipReason::Occluded),"native frame was not presented or explicitly deferred"); return result;
    };
    auto capture=[&](const std::string& name) {
        auto result=frame(true); Require(result.screenshot.has_value(),"capture missing"); const auto& image=*result.screenshot;
        Assets::WritePng(output/(name+".png"),int(image.extent.width),int(image.extent.height),4,image.pixels); return image.pixels;
    };
    const auto original=capture("original");
    if(mode.starts_with("async-")) {
        (void)r.PollGpuSamples();
        if(mode=="async-gated") {
            Gate gate(*d); const auto result=frame(); const auto waits=d->wait_calls,queries=d->query_calls;
            Require(gate.Status()==VK_NOT_READY&&gate.Counter()==0,"async probe has no actual pending frame");
            std::cout<<"BATCH4_POLL_BEGIN"<<std::endl;
            for(int i=0;i<10;++i)Require(r.PollGpuSamples().empty(),"unready timestamp fabricated a sample");
            std::cout<<"BATCH4_POLL_END"<<std::endl;
            Require(gate.Status()==VK_NOT_READY&&gate.Counter()==0&&d->wait_calls==waits&&d->query_calls==queries,"unready Poll waited or read a pending query");
            gate.Close(); const auto samples=r.PollGpuSamples();
            Require(samples.size()==1&&samples[0].frame_id==result.frame_id&&samples[0].valid&&samples[0].gpu_draw_time&&samples[0].missing_reason==GpuSampleMissingReason::None,"completed query lost original frame ID");
            Require(r.PollGpuSamples().empty(),"query was returned twice");
            report["pending_poll_calls"]=10; report["pending_fence"]=int(VK_NOT_READY); report["pending_timeline"]=0;
            report["poll_added_waits"]=d->wait_calls-waits; report["resolved_frame_id"]=result.frame_id;
            report["actual_in_flight"]=true;
        } else if(mode=="async-overflow") {
            std::set<std::uint64_t> submitted,returned;
            for(int i=0;i<300;++i)submitted.insert(frame().frame_id);
            submitted.insert(frame(true).frame_id);
            const auto samples=r.PollGpuSamples(); Require(samples.size()==256,"ready ring did not enforce 256 capacity");
            for(const auto& sample:samples) {
                Require(submitted.contains(sample.frame_id)&&returned.insert(sample.frame_id).second&&sample.valid&&sample.gpu_draw_time&&sample.missing_reason==GpuSampleMissingReason::None,"overflow returned invalid/duplicate sample");
                report["returned_ids"].push_back(sample.frame_id);
            }
            for(auto value:submitted)report["submitted_ids"].push_back(value);
            Require(r.PollGpuSamples().empty(),"overflow ring did not drain");
        } else if(mode=="async-unsupported") {
            Require(!r.GetInfo().capabilities.gpu_timestamps,"injected unsupported capability was not reflected");
            const auto result=frame(); const auto samples=r.PollGpuSamples();
            Require(samples.size()==1&&samples[0].frame_id==result.frame_id&&!samples[0].valid&&!samples[0].gpu_draw_time&&samples[0].missing_reason==GpuSampleMissingReason::Unsupported&&d->query_calls==0,"unsupported timing fabricated a duration");
            report["simulated_unsupported"]=true; report["missing_reason"]="unsupported"; report["missing_has_duration"]=false;
        } else if(mode=="async-discarded") {
            const auto hwnd=Platform::NativeBridge::GetHandle(*owner.window); ShowWindow(hwnd,SW_MINIMIZE);
            auto deadline=Clock::now()+std::chrono::seconds(2);
            do {owner.window->PollInt();owner.window->WaitEvents(.01);} while(!owner.window->Minimized()&&Clock::now()<deadline);
            Require(IsIconic(hwnd)&&owner.window->Minimized(),"native minimize did not occur");
            FrameInput input; input.frame_id=id++; input.camera.position={0,0,5}; input.draws=draws;
            const auto result=r.Render(input); const auto samples=r.PollGpuSamples();
            Require(result.status==FrameStatus::Skipped&&result.skip_reason==FrameSkipReason::ZeroExtent&&samples.size()==1&&samples[0].frame_id==input.frame_id&&!samples[0].valid&&!samples[0].gpu_draw_time&&samples[0].missing_reason==GpuSampleMissingReason::Discarded,"native zero extent lacks discarded sample");
            ShowWindow(hwnd,SW_RESTORE); deadline=Clock::now()+std::chrono::seconds(2);
            do {owner.window->PollInt();owner.window->WaitEvents(.01);} while(owner.window->Minimized()&&Clock::now()<deadline);
            Require(!owner.window->Minimized(),"discard probe did not restore");
            r.Resize({unsigned(owner.window->width),unsigned(owner.window->height)});
            Require(capture("after-discard")==original,"discarded frame corrupted accepted resources");
            report["missing_reason"]="discarded"; report["missing_has_duration"]=false;
        } else if(mode=="async-device-error") {
            frame(); d->next_query=VK_ERROR_DEVICE_LOST; std::optional<RenderError> first;
            const auto deadline=Clock::now()+std::chrono::seconds(2);
            while(!first&&Clock::now()<deadline) {
                try {(void)r.PollGpuSamples();} catch(const RenderError& e){first=e;}
                if(!first)std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            Require(first&&first->Category()==RenderErrorCategory::DeviceLost&&first->NativeCode()=="-4","query error was converted to valid/zero GPU time");
            Failed(r,*d,*first,report); renderer.reset();
            Require(d->quarantined&&!d->native_destroyed,"query error owner was falsely retired");
            report["injected_query_device_lost"]=true; Stats(report,*d); return;
        }
        r.DestroyMesh(mesh); r.DestroyTextureArray(texture);
    } else if(mode=="lifetime") {
        for(unsigned step=0;step<3;++step) {
            Gate gate(*d); const auto before=Clock::now(); frame();
            const auto old_mesh=d->frame_mesh,old_texture=d->frame_texture;
            Require(gate.Status()==VK_NOT_READY&&gate.Counter()==0,"GPU already complete before mutation");
            const auto buffers=d->buffers_released,images=d->images_released;
            if(step==0) {
                r.UpdateMesh(mesh,Quad(1,1000));
                Require(old_mesh.owner_before(d->last_mesh)||d->last_mesh.owner_before(old_mesh),"update overwrote rather than replaced native version");
            }
            if(step==1) r.UpdateMesh(mesh,{});
            if(step==2) { r.DestroyMesh(mesh); r.DestroyTextureArray(texture); }
            std::vector<MeshHandle> capacity;
            for(unsigned i=0;i<48;++i)capacity.push_back(r.CreateMesh(Quad(0,i+1)));
            for(auto h:capacity)r.DestroyMesh(h);
            Require(!old_mesh.expired()&&!old_texture.expired()&&gate.Status()==VK_NOT_READY&&gate.Counter()==0,"in-flight native version released before completion");
            Value row; row["step"]=step; row["fence_before_and_after_mutation"]=int(VK_NOT_READY);
            row["old_mesh_retained"]=true; row["old_texture_retained"]=true; row["buffers_released_before"]=buffers;
            row["timeline_before_and_after_mutation"]=0;
            row["diagnostic_deferred_present"]=true;
            row["images_released_before"]=images; row["peak_bytes"]=d->peak_bytes;
            gate.Close(); Require(gate.Status()==VK_SUCCESS,"gate release did not complete real frame");
            if(step==2) {
                Expect([&]{r.DestroyMesh(mesh);},RenderErrorCategory::InvalidHandle);
                Expect([&]{r.DestroyTextureArray(texture);},RenderErrorCategory::InvalidHandle);
                draws.clear();
            }
            for(int i=0;i<3;++i)frame();
            Require(old_mesh.expired(),"completed old mesh version was not retired on slot reuse");
            if(step==2)Require(old_texture.expired(),"completed deleted texture was not retired");
            row["old_mesh_retired"]=true; row["deleted_texture_retired"]=step==2;
            row["elapsed_ms"]=std::chrono::duration<double,std::milli>(Clock::now()-before).count(); report["events"].push_back(row);
            if(step==0)Require(capture("updated")!=original,"updated texture layer not visible");
            if(step==1) { Require(capture("empty")!=original,"empty mesh retained original geometry"); r.UpdateMesh(mesh,Quad()); Require(capture("restored")==original,"restored mesh differs from original"); }
        }
        report["capacity_creates"]=144; report["actual_in_flight"]=true;
    } else if(mode=="allocation"||mode=="host-allocation") {
        const auto old=d->last_mesh; const auto before=d->live_bytes;
        if(mode=="allocation")d->next_allocation=VK_ERROR_OUT_OF_DEVICE_MEMORY; else d->next_host_allocation=true;
        auto e=Expect([&]{r.UpdateMesh(mesh,Quad(1,2000));},RenderErrorCategory::AllocationFailure);
        report["first_operation"]=e.Operation(); report["first_native_code"]=e.NativeCode();
        Require(!old.expired()&&d->live_bytes==before,"failed update published or leaked candidate");
        Require(capture("after-failed-update")==original,"recoverable failed update lost original pixels");
        const auto before_create=d->live_bytes;
        if(mode=="allocation")d->next_allocation=VK_ERROR_OUT_OF_DEVICE_MEMORY; else d->next_host_allocation=true;
        Expect([&]{(void)r.CreateMesh(Quad(1,2000));},RenderErrorCategory::AllocationFailure);
        Require(d->live_bytes==before_create,"failed Create leaked an unpublished native candidate");
        const auto retried=r.CreateMesh(Quad(1,2000)); r.DestroyMesh(retried);
        report["failed_create_rolled_back"]=true;
        r.UpdateMesh(mesh,Quad(1)); Require(capture("successful-retry")!=original,"update retry failed");
        report["old_contents_preserved"]=true; r.DestroyMesh(mesh); r.DestroyTextureArray(texture);
    } else {
        if(mode=="submit") d->next_submit=VK_ERROR_UNKNOWN;
        if(mode=="device-lost") d->next_present=VK_ERROR_DEVICE_LOST;
        std::unique_ptr<Gate> gate;
        if(mode=="texture-timeout"||mode=="frame-timeout")gate=std::make_unique<Gate>(*d);
        const auto e=Expect([&] {
            if(mode=="texture-timeout"){(void)r.CreateTextureArray(Texture());}
            else if(mode=="frame-timeout"){frame(true);} else frame();
        },mode=="submit"?RenderErrorCategory::SubmissionFailure:mode=="device-lost"?RenderErrorCategory::DeviceLost:RenderErrorCategory::Timeout);
        if(gate) {
            Require(gate->Status()==VK_NOT_READY&&gate->Counter()==0,"timeout had no actual pending GPU work");
            report["actual_in_flight"]=true; report["fence_at_timeout"]=int(VK_NOT_READY); report["timeline_at_timeout"]=0;
        }
        Failed(r,*d,e,report); renderer.reset();
        Require(d->quarantined&&!d->native_destroyed&&!d->loader_released&&d->live_bytes>0,"failed native owner/loader not retained");
        const auto attempts=d->submit_attempts,presents=d->present_calls;
        if(gate){gate->Close();report["owned_work_completed_after_failure"]=true;}
        Require(d->submit_attempts==attempts&&d->present_calls==presents,"terminal gate cleanup resumed native submission/presentation");
        Require(!d->native_destroyed&&!d->loader_released,"gate completion incorrectly reclassified quarantine");
        Stats(report,*d); return;
    }
    r.Shutdown(); r.Shutdown(); renderer.reset();
    Require(d->native_destroyed&&d->loader_released&&!d->quarantined&&d->live_bytes==0&&d->buffers_created==d->buffers_released&&d->images_created==d->images_released,"normal versions/native owner not fully reclaimed");
    Stats(report,*d);
}
void Game(const std::string& mode,Value& report) {
    auto d=std::make_shared<VulkanDiagnosticAccess>(); std::unique_ptr<Gate> gate;
    d->enable_timeline_gate=mode=="game-timeout";
    if(mode=="game-allocation")d->next_host_allocation=true;
    if(mode=="game-submit")d->on_initialized=[](auto& c){c.next_submit=VK_ERROR_UNKNOWN;};
    if(mode=="game-device-lost")d->next_present=VK_ERROR_DEVICE_LOST;
    if(mode=="game-timeout")d->on_initialized=[&](auto& c){gate=std::make_unique<Gate>(c);};
    ScopedVulkanDiagnostics binding(d);
    std::vector<std::string> args{"SymoCraft","--renderer","vulkan","--msaa","1","--validation","1","--seed","424242","--scene","regression","--smoke-frames","2"};
    std::vector<char*> argv; for(auto& a:args)argv.push_back(a.data());
    const auto code=Application::RunGame(int(argv.size()),argv.data()); report["game_exit_code"]=code;
    Require(code==(mode=="game-normal"?0:3),"production game error exit classification changed");
    Application::Free(); report["repeat_application_free"]=true;
    if(gate) {
        Require(d->quarantined&&gate->Status()==VK_NOT_READY&&gate->Counter()==0,"game texture timeout was not actually pending/retained");
        gate->Close(); report["actual_in_flight"]=true; report["owned_work_completed_after_failure"]=true;
        report["fence_at_timeout"]=int(VK_NOT_READY); report["timeline_at_timeout"]=0;
    }
    if(mode=="game-normal"||mode=="game-allocation")Require(d->native_destroyed&&d->loader_released&&d->live_bytes==0,"game cleanup failed");
    else Require(d->quarantined&&!d->native_destroyed&&!d->loader_released,"game fatal cleanup falsely retired unproven owner");
    if(d->quarantined) {
        HMODULE module{};
        Require(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(d->get_device_proc),&module)!=0&&reinterpret_cast<std::uintptr_t>(module)==d->loader_address,
            "actual loader was not mapped after Application freed Window");
        report["actual_loader_mapped_after_window_free"]=true;
    }
    {OwnedWindow clean; report["platform_reinitialized"]=true;}
    Stats(report,*d);
}
}
int main(int argc,char** argv) {
    if(argc!=3)return 64; const std::string mode=argv[1];
    const std::array modes{"lifetime","allocation","host-allocation","pipeline","submit","device-lost","texture-timeout","frame-timeout","game-normal","game-allocation","game-submit","game-device-lost","game-timeout",
        "async-gated","async-overflow","async-unsupported","async-discarded","async-device-error"};
    if(std::find(modes.begin(),modes.end(),mode)==modes.end())return 64;
    const auto output=std::filesystem::absolute(argv[2]); if(std::filesystem::exists(output))return 64; std::filesystem::create_directories(output);
    Value report; report["schema"]="symocraft.vulkan-production-lifetime-fault.v1"; report["mode"]=mode; report["success"]=false;
    report["spontaneous_hardware_device_loss_or_real_oom"]=false; report["public_facade_and_production_backend"]=true;
    report["production_game_entry"]=mode.starts_with("game-"); const auto start=Clock::now(); int code=1;
    report["fault_kind"]=mode=="submit"||mode=="game-submit"?"injected submit result before native call":
        mode=="device-lost"||mode=="game-device-lost"?"injected DeviceLost after actual native Present":
        mode=="texture-timeout"||mode=="frame-timeout"||mode=="game-timeout"?"actual gated native fence timeout":
        mode=="allocation"||mode=="host-allocation"||mode=="pipeline"||mode=="game-allocation"?"injected allocation/pipeline result":"normal controlled native work";
    if(mode=="async-device-error")report["fault_kind"]="injected query result after actual query success";
    if(mode=="async-unsupported")report["fault_kind"]="injected timestamp capability off; actual GPU drawing retained";
    try { if(mode.starts_with("game-"))Game(mode,report); else Facade(mode,output,report); report["success"]=true; code=0; }
    catch(const std::exception& e){report["harness_error"]=e.what();std::cerr<<"[probe] "<<e.what()<<'\n';}
    report["elapsed_ms"]=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
    std::ofstream file(output/"report.json"); Json(file,report); file<<'\n'; return file?code:2;
}
