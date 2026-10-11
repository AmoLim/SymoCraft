SymoCraft production transition game candidate (Release, Windows x64)

The actual transition game keeps legacy OpenGL as the default; D3D12 is selected
explicitly. Both use one game loop and full game assets. Preserve every package
byte and package-manifest.json. Store run evidence outside this immutable package.

From the package directory:
  .\SymoCraft.exe --check-assets
  .\SymoCraft.exe --check-assets --renderer d3d12
  .\SymoCraft.exe
  .\SymoCraft.exe --renderer d3d12 --msaa 4 --validation 1

D3D12 requires a real capable hardware adapter and Windows Graphics Tools for the
Debug Layer. No WARP, backend fallback, or silent validation disablement is used.
Compiled DXIL and app-local matching MSVC CRT are included; no DXC, Python,
compiler, development SDK, SDL DLL, or source checkout is needed at runtime.
Normal Windows graphics libraries and installed hardware drivers remain required.
Debug Layer binaries are not redistributed here.

Runtime policy: app-local-redistributable.
Debug CRT files come from Debug_NonRedist and the selected SDK's debug UCRT. Debug
candidates are for this developer's local diagnosis only, not external
redistribution. Use the Release candidate for portable review.

provenance/candidate-build-identities.json binds the actual successful production
link, executable, four shaders, selected production source trees, and build tools.
Builder paths are provenance only, not runtime lookup paths. dependencies.json
and import logs record actual static imports, not a clean-machine runtime test or
a complete dynamic driver dependency inventory. Package hashes are relative.

This is R2 sequence 4 candidate delivery, not R3 acceptance or approval of the
14-operation / 15-minute manual matrix. Packaging implies no manual result.
