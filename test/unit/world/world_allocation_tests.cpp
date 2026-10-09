#include "world_test_access.h"
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <new>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#include <crtdbg.h>

namespace {
    struct Header { void* allocation; std::size_t bytes; };
    std::atomic<std::size_t> live_count{}, live_bytes{};
    thread_local std::int64_t fail_after = -1;
    thread_local std::size_t attempts = 0;
    const char* phase = "startup";
    void PrintStack() noexcept
    {
        void* frames[40]{};
        const auto count = CaptureStackBackTrace(0,40,frames,nullptr);
        const auto process = GetCurrentProcess();
        for (USHORT i = 0; i < count; ++i) {
            alignas(SYMBOL_INFO) unsigned char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
            auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
            DWORD64 displacement{};
            if (SymFromAddr(process,reinterpret_cast<DWORD64>(frames[i]),&displacement,symbol))
                std::fprintf(stderr,"  %s + 0x%llx\n",symbol->Name,static_cast<unsigned long long>(displacement));
            else std::fprintf(stderr,"  %p\n",frames[i]);
        }
    }
    void Terminated() noexcept
    {
        std::fprintf(stderr,"terminate: phase=%s, fail_after=%lld, attempts=%zu\n",phase,
            static_cast<long long>(fail_after),attempts);
        try { if (const auto exception = std::current_exception()) std::rethrow_exception(exception); }
        catch (const std::exception& error) { std::fprintf(stderr,"active exception: %s\n",error.what()); }
        catch (...) { std::fprintf(stderr,"active exception: non-standard\n"); }
        PrintStack();
        std::fflush(stderr);
        std::_Exit(134);
    }
    LONG WINAPI Unhandled(EXCEPTION_POINTERS* exception) noexcept
    {
        std::fprintf(stderr,"unhandled Windows exception: code=0x%08lx, phase=%s, fail_after=%lld, attempts=%zu\n",
            exception->ExceptionRecord->ExceptionCode,phase,static_cast<long long>(fail_after),attempts);
        std::fflush(stderr);
        std::_Exit(135);
    }
    void InvalidParameter(const wchar_t* expression,const wchar_t* function,const wchar_t* file,unsigned int line,std::uintptr_t) noexcept
    {
        std::fprintf(stderr,"CRT invalid parameter: %ls, function=%ls, file=%ls:%u, phase=%s, attempts=%zu\n",
            expression ? expression : L"",function ? function : L"",file ? file : L"",line,phase,attempts);
        PrintStack(); std::fflush(stderr); std::_Exit(136);
    }
    void* Allocate(std::size_t bytes, std::size_t alignment)
    {
        ++attempts;
        if (fail_after == 0) throw std::bad_alloc();
        if (fail_after > 0) --fail_after;
        alignment = std::max(alignment,alignof(std::max_align_t));
        const auto prefix = (sizeof(Header) + alignment - 1) & ~(alignment - 1);
        auto* allocation = static_cast<unsigned char*>(_aligned_malloc(prefix + (bytes ? bytes : 1),alignment));
        if (!allocation) throw std::bad_alloc();
        auto* result = allocation + prefix;
        auto* header = reinterpret_cast<Header*>(result) - 1;
        *header = {allocation,bytes};
        ++live_count; live_bytes += bytes;
        return result;
    }
    void Release(void* pointer) noexcept
    {
        if (!pointer) return;
        const auto* header = static_cast<Header*>(pointer) - 1;
        --live_count; live_bytes -= header->bytes;
        _aligned_free(header->allocation);
    }
    struct FailAllocation {
        explicit FailAllocation(std::size_t index) { attempts = 0; fail_after = static_cast<std::int64_t>(index); }
        ~FailAllocation() { fail_after = -1; }
    };
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template<typename Callback> std::size_t ExhaustFailurePoints(Callback callback, const char* message)
    {
        phase = message;
        for (std::size_t point = 0; point < 4096; ++point) {
            if (point % 256 == 0) std::fprintf(stderr,"failure sweep %s: ordinal=%zu\n",message,point);
            const auto count_before = live_count.load(), bytes_before = live_bytes.load();
            bool failed = false;
            {
                FailAllocation failure(point);
                try { callback(); } catch (const std::bad_alloc&) { failed = true; }
            }
            if (live_count != count_before || live_bytes != bytes_before)
                std::fprintf(stderr,"allocation failure ordinal=%zu; live before=%zu/%zu bytes, after=%zu/%zu bytes\n",
                    point,count_before,bytes_before,live_count.load(),live_bytes.load());
            Require(live_count == count_before && live_bytes == bytes_before,message);
            if (!failed) return point;
        }
        throw std::runtime_error("Allocation failure sweep did not reach success");
    }
}

void* operator new(std::size_t bytes) { return Allocate(bytes,alignof(std::max_align_t)); }
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* pointer) noexcept { Release(pointer); }
void operator delete[](void* pointer) noexcept { Release(pointer); }
void operator delete(void* pointer,std::size_t) noexcept { Release(pointer); }
void operator delete[](void* pointer,std::size_t) noexcept { Release(pointer); }
void* operator new(std::size_t bytes,std::align_val_t alignment) { return Allocate(bytes,static_cast<std::size_t>(alignment)); }
void* operator new[](std::size_t bytes,std::align_val_t alignment) { return ::operator new(bytes,alignment); }
void operator delete(void* pointer,std::align_val_t) noexcept { Release(pointer); }
void operator delete[](void* pointer,std::align_val_t) noexcept { Release(pointer); }
void operator delete(void* pointer,std::size_t,std::align_val_t) noexcept { Release(pointer); }
void operator delete[](void* pointer,std::size_t,std::align_val_t) noexcept { Release(pointer); }

int main(int argc,char** argv)
{
    using namespace SymoCraft;
    using namespace SymoCraft::World;
    try {
        std::set_terminate(Terminated);
        SetUnhandledExceptionFilter(Unhandled);
        _set_invalid_parameter_handler(InvalidParameter);
        _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        SymInitialize(GetCurrentProcess(),nullptr,TRUE);
        Require(argc == 2,"Expected real configuration path");
        std::ifstream input(argv[1],std::ios::binary);
        Require(static_cast<bool>(input),"Cannot read configuration");
        const std::string text{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto definition = BlockDefinition::FromConfig(text);
        auto other = VoxelWorld::Create(definition,{424243,2,false});
        other->RebuildDirtyMeshes();
        const auto other_digest = other->Digest();
        const auto definitions = ExhaustFailurePoints([&] { static_cast<void>(BlockDefinition::FromConfig(text)); },"Definition allocation failure leaked C++ owned resources");
        const auto factories = ExhaustFailurePoints([&] { static_cast<void>(VoxelWorld::Create(definition,{424242,2,true})); },"Factory/Chunk/noise allocation failure leaked C++ owned resources");
        const auto generators = ExhaustFailurePoints([&] { const Generation::Generator generator({424242,2,true}); },"Generator/noise allocation failure leaked C++ owned resources");
        std::size_t mesh_points = 0;
        for (std::size_t point = 0; point < 1024; ++point) {
            const auto before_worlds = live_count.load(), before_bytes = live_bytes.load();
            bool failed = false;
            {
                auto world = VoxelWorld::Create(definition,{424242,2,false});
                WorldTestAccess::ResetToAir(*world);
                for (int y = 1; y < 255; y += 2)
                    for (int x = 1; x < 15; x += 2)
                        for (int z = 1; z < 15; z += 2)
                            world->TryEdit({EditOperation::Set,{x,y,z},2});
                {
                    FailAllocation failure(point);
                    try { world->RebuildDirtyMeshes(); } catch (const std::bad_alloc&) { failed = true; }
                }
                if (failed) {
                    const auto& chunk = WorldTestAccess::Chunk(*world,{0,0});
                    Require(!chunk.published_mesh_revision && chunk.mesh.vertices.empty(),"Scratch/output allocation failure published partial mesh");
                    Require(world->RebuildDirtyMeshes() > 0,"Same mesher cannot recover from real allocation failure");
                    Require(chunk.mesh.vertices.size() == 127 * 49 * 36,"Allocation retry retained scratch residue or lost faces");
                }
            }
            Require(live_count == before_worlds && live_bytes == before_bytes,"Scratch/candidate allocation failure leaked C++ resources after World destruction");
            if (!failed) { mesh_points = point; break; }
            Require(point < 1023,"Mesh failure sweep did not reach success");
        }
        Require(other->Digest() == other_digest && other->RebuildDirtyMeshes() == 0,"Allocation failure polluted another world");
        other->VisitMeshes([](const WorldMeshRecord& record) { if (record.vertices.size_bytes() % sizeof(BlockVertex3D)) throw std::runtime_error("Other mesh corrupted"); });
        std::cout << "Real C++ allocation failure sweep passed: definition=" << definitions << "; factory=" << factories
            << "; generator/noise=" << generators << "; scratch/output=" << mesh_points
            << ". Every failed ordinal released tracked resources after owner destruction; retries used same mesher.\n";
        return 0;
    } catch (const std::exception& error) { fail_after = -1; std::cerr << error.what() << '\n'; return 1; }
}
