"""CPU-only contract tests for candidate packaging, verification and window probes."""

import ast
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


HERE = Path(__file__).resolve().parent
REPOSITORY = HERE.parents[2]


def import_script(name):
    directory = REPOSITORY / "scripts" if name == "package-d3d12-candidate" else HERE
    spec = importlib.util.spec_from_file_location(name.replace("-", "_"), directory / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PACKAGE = import_script("package-d3d12-candidate")
VERIFY = import_script("verify-d3d12-candidate")
WINDOW = import_script("verify-d3d12-window")


class CandidateScriptsTests(unittest.TestCase):
    def test_runtime_environment_accepts_windows_uppercase_keys(self):
        environment = {"SYSTEMROOT": str(REPOSITORY), "PATH": "developer-tools", "INCLUDE": "sdk",
                       "WindowsSDKVersion": "sdk-version", "VCToolsInstallDir": "compiler"}
        with mock.patch.dict(VERIFY.os.environ, environment, clear=True):
            result = VERIFY.runtime_environment()
        self.assertEqual(result["SYSTEMROOT"], str(REPOSITORY))
        self.assertNotIn("INCLUDE", result)
        self.assertNotIn("WindowsSDKVersion", result)
        self.assertNotIn("VCToolsInstallDir", result)
        self.assertNotIn("developer-tools", result["PATH"])

    def test_ninja_readiness_allows_only_verification_and_asset_sync(self):
        for text in ("ninja: no work to do.\n",
                     "[0/2] Re-checking globbed directories...\n[1/2] Synchronizing required SymoCraft runtime assets\n"):
            self.assertTrue(PACKAGE.readiness_is_settled(0, text))
        for text in ("", "[1/2] Re-running CMake...\n", "[1/2] Linking CXX executable bin/SymoCraft.exe\n",
                     "[1/2] Building CXX object app.obj\n", "[1/2] Recording the production candidate executable and build identities\n"):
            self.assertFalse(PACKAGE.readiness_is_settled(0, text))
        self.assertFalse(PACKAGE.readiness_is_settled(1, "ninja: no work to do.\n"))

    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="symocraft-candidate-cpu-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name).resolve()
        self.allowed = self.root / "out" / "m3-t3"
        self.allowed.mkdir(parents=True)
        self.package = self.allowed / "candidate with spaces"
        self.package.mkdir()
        for module, name, value in ((PACKAGE, "ROOT", self.root), (PACKAGE, "ALLOWED_ROOT", self.allowed),
                                    (VERIFY, "ROOT", self.root), (VERIFY, "ALLOWED", self.allowed),
                                    (WINDOW, "__file__", str(self.root / "test/integration/renderer/window.py"))):
            patch = mock.patch.object(module, name, value)
            patch.start()
            self.addCleanup(patch.stop)
        launch = mock.patch.object(WINDOW.subprocess, "Popen", side_effect=AssertionError("CPU tests must not launch a game."))
        launch.start()
        self.addCleanup(launch.stop)

    def payload(self, relative, data=b"cpu-fixture-only"):
        path = self.package / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def valid_manifest(self, configuration="Release"):
        for name in VERIFY.REQUIRED_FILES:
            self.payload(name, ("not an executable or GPU asset: " + name).encode("ascii"))
        self.payload("README.txt", b"CPU manifest fixture; no executable is run.\n")
        files = []
        for path in sorted(self.package.rglob("*")):
            if path.is_file() and path.name != "package-manifest.json":
                entry = VERIFY.identity(path)
                entry["path"] = path.relative_to(self.package).as_posix()
                files.append(entry)
        manifest = {
            "schema_version": 1, "package_kind": "production-game-candidate", "configuration": configuration,
            "default_renderer": "opengl", "available_renderers": ["opengl", "d3d12"],
            "required_debug_layer": True, "validation_default": True, "software_adapter_allowed": False,
            "runtime_policy": "developer-local-nonredistributable" if configuration == "Debug" else "app-local-redistributable",
            "files": files,
        }
        self.write_manifest(manifest)
        return manifest

    def write_manifest(self, manifest):
        (self.package / "package-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")

    def marked_reparse(self, selected):
        original = Path.lstat

        def lstat(path, *args, **kwargs):
            value = original(path, *args, **kwargs)
            if path == selected:
                return SimpleNamespace(st_mode=value.st_mode, st_file_attributes=0x400)
            return value

        return mock.patch.object(Path, "lstat", new=lstat)

    def window_options(self, output):
        return SimpleNamespace(package_directory=str(self.package), output_directory=str(output))

    def test_package_file_identity_is_exact(self):
        path = self.payload("file with spaces.txt", b"\x00\x01\xff")
        item = PACKAGE.file_identity(path, "file with spaces.txt")
        self.assertEqual(item, {"path": "file with spaces.txt", "bytes": 3,
                                "sha256": hashlib.sha256(b"\x00\x01\xff").hexdigest()})
        PACKAGE.assert_identity(path, item, "CPU fixture")

    def test_package_identity_rejects_empty_directory_and_changed_bytes(self):
        empty = self.payload("empty.txt", b"")
        with self.assertRaises(ValueError):
            PACKAGE.file_identity(empty)
        self.assertEqual(PACKAGE.file_identity(empty, allow_empty=True)["bytes"], 0)
        with self.assertRaises(ValueError):
            PACKAGE.file_identity(self.package)
        payload = self.payload("change.txt", b"one")
        item = PACKAGE.file_identity(payload)
        payload.write_bytes(b"two")
        with self.assertRaises(ValueError):
            PACKAGE.assert_identity(payload, item, "CPU fixture")

    def test_package_scope_normalizes_and_rejects_escape(self):
        accepted = PACKAGE.workspace_path("out/m3-t3/new-package")
        PACKAGE.assert_child(accepted, self.allowed)
        for relative in ("out/m3-t3", "out/m3-t3/../escaped", "out/m3-t3-other/candidate"):
            with self.subTest(relative=relative), self.assertRaises(ValueError):
                PACKAGE.assert_child(PACKAGE.workspace_path(relative), self.allowed)

    def test_package_reparse_ancestor_is_rejected(self):
        with self.marked_reparse(self.package), self.assertRaises(ValueError):
            PACKAGE.assert_child(self.package / "new-child", self.allowed)

    def test_package_file_reparse_is_rejected(self):
        path = self.payload("redirected.txt")
        with self.marked_reparse(path), self.assertRaises(ValueError):
            PACKAGE.file_identity(path)

    def test_manifest_release_and_debug_round_trip(self):
        for configuration in ("Release", "Debug"):
            with self.subTest(configuration=configuration):
                expected = self.valid_manifest(configuration)
                self.assertEqual(VERIFY.read_package_manifest(self.package), expected)

    def test_manifest_all_paths_are_relative_and_file_set_is_exact(self):
        manifest = self.valid_manifest()
        actual = VERIFY.package_identity(self.package)
        self.assertEqual({item["path"] for item in actual},
                         {item["path"] for item in manifest["files"]} | {"package-manifest.json"})
        self.assertTrue(all(not Path(item["path"]).is_absolute() for item in actual))

    def test_modern_only_manifest_preserves_default_and_excludes_legacy_payload(self):
        manifest = self.valid_manifest()
        manifest["available_renderers"] = ["d3d12"]
        self.write_manifest(manifest)
        self.assertEqual(VERIFY.read_package_manifest(self.package), manifest)
        for name in ("assets/shaders/blocksVertexShader.glsl", "licenses/glad-LICENSE.txt"):
            path = self.payload(name)
            entry = VERIFY.identity(path)
            entry["path"] = name
            manifest["files"].append(entry)
            self.write_manifest(manifest)
            with self.assertRaises(RuntimeError):
                VERIFY.read_package_manifest(self.package)

    def test_manifest_rejects_duplicate_and_case_alias(self):
        original = self.valid_manifest()
        for alias in ("SymoCraft.exe", "symocraft.exe"):
            with self.subTest(alias=alias):
                manifest = copy.deepcopy(original)
                duplicate = copy.deepcopy(next(item for item in manifest["files"] if item["path"] == "SymoCraft.exe"))
                duplicate["path"] = alias
                manifest["files"].append(duplicate)
                self.write_manifest(manifest)
                with self.assertRaises(RuntimeError):
                    VERIFY.read_package_manifest(self.package)

    def test_manifest_rejects_parent_absolute_and_noncanonical_paths(self):
        original = self.valid_manifest()
        invalid = ("../escaped.txt", "assets/../SymoCraft.exe", "./SymoCraft.exe", "assets\\texture.png",
                   str(self.package / "SymoCraft.exe"), "package-manifest.json", "")
        for relative in invalid:
            with self.subTest(relative=relative):
                manifest = copy.deepcopy(original)
                manifest["files"][0]["path"] = relative
                self.write_manifest(manifest)
                with self.assertRaises(RuntimeError):
                    VERIFY.read_package_manifest(self.package)

    def test_manifest_rejects_changed_hash_and_size(self):
        original = self.valid_manifest()
        for key, replacement in (("sha256", "0" * 64), ("bytes", 123456)):
            with self.subTest(key=key):
                manifest = copy.deepcopy(original)
                manifest["files"][0][key] = replacement
                self.write_manifest(manifest)
                with self.assertRaises(RuntimeError):
                    VERIFY.read_package_manifest(self.package)

    def test_manifest_rejects_changed_payload(self):
        self.valid_manifest()
        (self.package / "SymoCraft.exe").write_bytes(b"different fixture bytes")
        with self.assertRaises(RuntimeError):
            VERIFY.read_package_manifest(self.package)

    def test_manifest_rejects_missing_and_added_payload(self):
        manifest = self.valid_manifest()
        path = self.package / "README.txt"
        original_bytes = path.read_bytes()
        path.unlink()
        with self.assertRaises(RuntimeError):
            VERIFY.read_package_manifest(self.package)
        path.write_bytes(original_bytes)
        self.write_manifest(manifest)
        self.payload("unexpected.txt")
        with self.assertRaises(RuntimeError):
            VERIFY.read_package_manifest(self.package)

    def test_manifest_rejects_undeclared_required_file(self):
        manifest = self.valid_manifest()
        (self.package / "SymoCraft.exe").unlink()
        manifest["files"] = [item for item in manifest["files"] if item["path"] != "SymoCraft.exe"]
        self.write_manifest(manifest)
        with self.assertRaises(RuntimeError):
            VERIFY.read_package_manifest(self.package)

    def test_manifest_accepts_current_and_historical_game_defaults(self):
        for default in ("opengl", "d3d12"):
            with self.subTest(default=default):
                manifest = self.valid_manifest()
                manifest["default_renderer"] = default
                self.write_manifest(manifest)
                self.assertEqual(VERIFY.read_package_manifest(self.package)["default_renderer"], default)

    def test_manifest_rejects_backend_default_validation_and_policy_changes(self):
        original = self.valid_manifest()
        changes = (("schema_version", 2), ("package_kind", "experiment"), ("configuration", "Unknown"),
                   ("default_renderer", "vulkan"), ("available_renderers", ["opengl", "d3d12", "vulkan"]),
                   ("required_debug_layer", False), ("validation_default", False),
                   ("software_adapter_allowed", True), ("runtime_policy", "developer-local-nonredistributable"))
        for key, replacement in changes:
            with self.subTest(key=key):
                manifest = copy.deepcopy(original)
                manifest[key] = replacement
                self.write_manifest(manifest)
                with self.assertRaises(RuntimeError):
                    VERIFY.read_package_manifest(self.package)

    def test_verifier_reparse_package_and_payload_are_rejected(self):
        self.valid_manifest()
        for selected in (self.package, self.package / "SymoCraft.exe"):
            with self.subTest(selected=selected), self.marked_reparse(selected), self.assertRaises(RuntimeError):
                VERIFY.read_package_manifest(self.package)

    def test_verifier_outside_path_is_rejected(self):
        for path in (self.root / "outside", self.allowed.parent / "m3-t3-other"):
            with self.subTest(path=path), self.assertRaises(RuntimeError):
                VERIFY.checked_path(path, scoped=True)

    def test_window_paths_reject_root_escape_and_reparse(self):
        WINDOW.checked_path(str(self.allowed / "new-window-run"), self.allowed)
        for path in (self.allowed, self.root / "outside", self.allowed.parent / "m3-t3-other"):
            with self.subTest(path=path), self.assertRaises(ValueError):
                WINDOW.checked_path(str(path), self.allowed)
        with self.marked_reparse(self.package), self.assertRaises(ValueError):
            WINDOW.checked_path(str(self.package / "new-child"), self.allowed)

    def test_window_output_rejects_existing_or_inside_candidate_before_launch(self):
        for path in (self.package, self.package / "new-run", self.allowed):
            with self.subTest(path=path), self.assertRaises(ValueError):
                WINDOW.verify(self.window_options(path))
        existing = self.allowed / "existing-run"
        existing.mkdir()
        with self.assertRaises(ValueError):
            WINDOW.verify(self.window_options(existing))

    def test_window_package_walk_rejects_redirected_entry(self):
        redirected = self.payload("redirected.txt")
        with self.marked_reparse(redirected), self.assertRaises(ValueError):
            WINDOW.package_files(self.package)

    def test_pe_parser_checks_actual_signature_and_x64_machine(self):
        payload = bytearray(70)
        payload[:2] = b"MZ"
        struct.pack_into("<I", payload, 60, 64)
        payload[64:68] = b"PE\0\0"
        struct.pack_into("<H", payload, 68, 0x8664)
        path = self.payload("cpu-pe-parser-fixture.bin", payload)
        PACKAGE.require_x64(path)
        for offset, changed in ((0, b"NO"), (64, b"NOPE"), (68, b"\x4c\x01")):
            with self.subTest(offset=offset):
                altered = bytearray(payload)
                altered[offset:offset+len(changed)] = changed
                path.write_bytes(altered)
                with self.assertRaises(ValueError):
                    PACKAGE.require_x64(path)
        path.write_bytes(b"MZ")
        with self.assertRaises(ValueError):
            PACKAGE.require_x64(path)

    def test_window_ordinary_arguments_match_existing_cli(self):
        tree = ast.parse((HERE / "verify-d3d12-window.py").read_text(encoding="utf-8"))
        verify = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "verify")
        assignment = next(node for node in verify.body if isinstance(node, ast.Assign)
                          and any(isinstance(target, ast.Name) and target.id == "arguments" for target in node.targets))
        arguments = ast.literal_eval(assignment.value)
        cpp = (REPOSITORY / "game/app/src/startup_options.cpp").read_text(encoding="utf-8")
        names = re.search(r"constexpr std::array names\{(.*?)\};", cpp, re.DOTALL)
        self.assertIsNotNone(names)
        allowed = set(re.findall(r'"(--[a-z-]+)"', names[1]))
        self.assertEqual(len(arguments) % 2, 0)
        selected = dict(zip(arguments[::2], arguments[1::2]))
        self.assertTrue(selected.keys() <= allowed)
        self.assertEqual(selected["--renderer"], "d3d12")
        self.assertEqual(selected["--scene"], "regression")
        self.assertEqual(selected["--checkpoint"], "four-chunk")
        self.assertTrue({"--benchmark", "--output", "--test-edits", "--smoke-frames"}.isdisjoint(selected))

    def test_window_bounded_integer_rejects_out_of_range(self):
        parse = WINDOW.bounded_integer(5, 60)
        self.assertEqual(parse("5"), 5)
        self.assertEqual(parse("60"), 60)
        for number in ("4", "61", "NaN"):
            with self.subTest(number=number), self.assertRaises((ValueError, WINDOW.argparse.ArgumentTypeError)):
                parse(number)


if __name__ == "__main__":
    unittest.main(verbosity=2)
