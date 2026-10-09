# CPU Contract Candidate

`candidate_contract.h` is an isolated proposal under
`SymoCraft::Experimental::D3D12R1::Candidate`, not a production public header.
The native GPU probe has a deliberately separate API. Neither API is Renderer v1.

The candidate reuses the real T1 `SymoCraft::MeshData` and `BlockVertex3D`.
Meshes remain nonindexed triangle lists, including valid empty results. Structural
validation checks complete triangles, finite UV/layer/normal values, and integer
nonnegative layers. A mesh's actual layer bound is checked when paired with a
texture array, not when created without a texture.

Texture input is owned RGBA8 data with positive dimensions/layers, top-left row
origin, tightly packed rows, and consecutive layers. Linear/sRGB is explicit;
sRGB describes RGB transfer, not alpha. Exact storage size and overflow are
checked before acceptance. No CPU byte conversion occurs in the registry.

The registry tests ownership on return, generational renderer-local opaque
tokens, transactional replacement, immediate logical invalidation, and stale,
foreign, and default-token rejection. Its snapshot getters return owned copies.
`IsNull()` only detects the default token; registry lookup establishes whether a
nondefault token is still live. It does not model fences, GPU lifetime, native
allocation failure, or device state. Texture update is a test-only extension and
is not implied to be part of the draft production facade.

`candidate_tests.cpp` requires C++20 and existing scene/foundation/GLM headers,
with no graphics SDK includes or graphics library linkage. The one-shot global
allocation override belongs only to this executable and injects CPU snapshot
allocation failure to verify preservation of previously accepted resources.

The standalone project now adds a declaration-only Renderer candidate consumer,
production handoff tests, and CPU dependency checks. It embeds the real root CPU
targets with game/benchmark/root tests disabled, and links assets, simulation and
world rather than recompiling substitute production sources. It does not discover
SDL, DXC, D3D12 or Vulkan or link any graphics library. MSVC still needs its normal
Windows C/C++ toolchain; foundation contains its existing Windows file support.

```powershell
./test/experimental/d3d12-r1/contract/build.ps1 -Configuration Debug `
  -BuildDirectory out/m3-t3/r1-cpu-handoff-debug-new
./test/experimental/d3d12-r1/contract/build.ps1 -Configuration Release `
  -BuildDirectory out/m3-t3/r1-cpu-handoff-release-new
```

`candidate_renderer.h` contains only CPU data, owning error text and the draft
PImpl facade declarations. No Renderer or RenderError runtime definitions are
provided; enum fields do not mean an implemented backend or reported capability.
The consumer compiles the method shapes and rejects SDK headers/backend macros.
The experimental header still includes the registry model: it must be split into
public data and private implementation before becoming a production public API.

`handoff_candidates.h` and its 82-check test consume the actual production atlas,
Camera and a small real World. They preserve bottom-row-first material layer IDs,
copy each layer to top-left RGBA8 storage, and propose one `1-v` sampling mapping
while leaving World UVs unchanged. They explicitly transfer degree FOV to radians,
near/far 0.1/2000, and compare RH_NO/RH_ZO camera projections numerically. Callback
records are copied immediately; the renderer-local mapping advances only after a
successful create/update, ignores newer dirty input revisions until published,
retains old accepted versions on failure, and releases only explicitly ended
worlds. World-space positions are not translated by chunk coordinates again.

CTest has four entries: registry, public consumer, production handoff and CPU
dependency boundaries. The latter examines cache, actual compiler commands and
Ninja link rules, not only interface target names. Debug/Release each pass 4/4.

Before a freeze, confirm the production GPU asset decoding and UV normalization path,
sampler filtering/wrap/mip semantics, right-handed camera basis construction,
FOV units, projection packing, and app ownership/version bookkeeping. Camera
parameters here accept finite nonzero, nonparallel forward/up directions and
radian vertical FOV. CPU numerical conversion does not prove GPU camera parity.
The old GL NEAREST/REPEAT, linear texture/framebuffer path differs from the
POINT/CLAMP, sRGB-RTV GPU fixture; production sampling/color/culling/depth still
requires explicit validation. Full-frame acceptance, frame-ID consumption, state,
thread/reentry and error translation rules are not frozen by the declarations.
Actual D3D12 lifetime/presentation needs separate GPU evidence. T2 acceptance,
the local RTX 5070 Ti R1 freeze review, and later whole-system performance gates
remain outside these CPU tests.
