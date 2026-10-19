# Symocraft Agent Guide

## Project
Windows x64 single-player voxel game, using C++20, MSVC, CMake/Ninja and SDL3.
The default game renderer is D3D12. Vulkan is an explicit opt-in backend;
legacy OpenGL 4.6 remains a transition path. T4 production sources are synchronized
to the main checkout, including SimulationSession and the independent CPU target
under simulation/physics; a formal top-level physics module is still deferred.

## Build And Run
Use an initialized x64 Visual Studio 2026 developer terminal with MSVC tools
14.51.36231 and Windows SDK 10.0.22621.0. Use a fresh directory when changing
compilers; retain old caches and results:
`cmake --preset windows-debug -B out/build/msvc51-debug`
`cmake --build out/build/msvc51-debug`
`ctest --test-dir out/build/msvc51-debug --output-on-failure --no-tests=error`
Run `out/build/msvc51-debug/bin/SymoCraft.exe`.
For ordinary terminals, use
`python scripts/build.py --configuration Debug --build-directory out/build/msvc51-debug`
with `--clion-path` or `CLION_HOME`. See `scripts/README.md` for prerequisites.
CPU-only uses a separate build directory and `-DSYMOCRAFT_CPU_ONLY=ON`.
D3D12 is explicitly enabled by the standard Debug/Release game presets, including
when reconfiguring old caches. Custom configurations can override it with
`-DSYMOCRAFT_RENDER_D3D12=OFF`; use `--renderer opengl` for the retained GL path.

## Ownership And Workflow
Production code belongs in `game/modules/` and `game/app/`; tests in `test/`;
the independent benchmark in `tools/benchmark/`. Respect CMake module boundaries.
Automation uses Python 3.10+ and the standard library. Do not add PowerShell
business logic or move native production logic into Python.
Read `docs/scope/project-scope.md` and `docs/DOCUMENTATION_MAP.md` first.
Current behavior belongs in `docs/openspec/specs/`; targets and sole active plans
belong in `docs/openspec/changes/`. Implementation mechanisms belong in
`docs/architecture/`; verification results in each change's `verification.md`;
key-node approvals and navigation only in `docs/verification/VERIFICATION.md`.
Retired legacy document directories have been removed; recovery is documented
in docs/verification/VERIFICATION.md. Missing baseline coverage does not mean missing implementation.
Preserve existing dirty changes, personal journal files, Obsidian settings,
immutable evidence bytes and historical commands/results. Necessary evidence
storage and retirement follow DOCUMENTATION_MAP: one verification.md per change,
no attachments or raw-data inclusion unless explicitly required. Existing frozen
evidence stays untouched; relocation is not byte rewriting.
Do not fill in
human verification checkboxes or delete residual files without confirmation.

## OpenSpec
The only planning directory is `docs/openspec/`; planning root is `docs`, while
the code root remains this project. Use `python tools/openspec/run.py <arguments>`
from the project root (use its absolute path from another directory). Generated
skill examples using `openspec` must also use this entry, or explicitly run in docs.
Native `actionContext.allowedEditRoots` lists docs only; never rewrite it to fake
code authorization. Before apply, independently confirm this turn's code-root scope.
Default `symocraft-lite` keeps short design and tasks in proposal.md; complex work
uses `symocraft-full` and tasks.md. No behavior change uses `skip_specs: true`.
Generated skills stay in root `.agents/skills`; refresh through the wrapper's
`update` command, not by hand or by creating docs/.agents or another openspec root.
Migration is not game implementation, approval, sync or archive authorization.
Before real sync/archive, confirm current authority, applicable checks and approvals;
CLI status/validate and checked tasks do not prove runtime correctness or acceptance.

## Proposal 前的规格预读
Every proposing or materially revising Agent, including delegated Agents, must:
1. Reread this file, DOCUMENTATION_MAP, scope and OpenSpec config; obtain current
   proposal instructions for the change through the project entry.
2. Inventory and read ALL current specs in full, including scenarios. An empty or
   partial baseline is not proof that the project has no other existing behavior.
3. Read related active changes' specs/design/decisions; check overlap and dependencies.
   Use retained migration sources only to recover unmapped knowledge, not as plans.
4. Identify added/modified/no behavior changes, reuse capability paths, and briefly
   link the basis in proposal. Do not create a reading report or read-specs artifact.
5. Report unreadable materials or design-blocking conflicts; reread affected content
   after concurrent specification edits. Another Agent's summary is not prereading.

## Git Rules
Commit, push, merge, PR, tags and history rewrites require current authorization;
old batch approvals never carry forward. Create `docs/git/gitspec.md` only for an
authorized current commit batch, replacing rather than appending old batch history.
There is no active Git batch during this migration; old gitspec is a review copy.
Group complete intentions and dependencies, not folders or arbitrary counts.
Inspect the exact staged diff; do not blindly stage the whole worktree or include
new concurrent changes. Keep user changes and secrets/private state protected.
Report unrun checks as unrun; historical results do not prove current validation
or intermediate commits. Do not change global Git configuration, credentials,
hooks or normalize the whole repository's line endings without authorization.
Keep required inputs visible in ignore rules and frozen bytes unchanged.
Commit messages follow `type(scope): Chinese description`.
