"""Offline archive boundary, byte identity and collector regression fixtures."""
import argparse
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import verification_common as common

candidate = common.load_script(ROOT / 'test/integration/renderer/archive-d3d12-candidate.py')
preparation = common.load_script(ROOT / 'test/experimental/sdl3/collect-evidence.py')
inputs = common.load_script(ROOT / 'test/experimental/sdl3/collect-input-evidence.py')


class ArchiveChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='evidence-tests-', dir=ROOT / 'out')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.patch = patch.object(common, 'ROOT', self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)

    def write(self, name, value=b'fixture'):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(value)
        return path

    def test_archive_root_and_legacy_destinations_rejected(self):
        for path in ('docs/verification/archive', 'docs/milestones/m3-t2/evidence/new',
                     'docs/milestones/m3-t3/evidence/r2/new', 'docs/verification/archive/../../outside', 'out/new'):
            with self.subTest(path=path), self.assertRaises(ValueError):
                common.archive_destination(path)
        self.assertEqual(common.archive_destination('docs/verification/archive/new'), self.root / 'docs/verification/archive/new')

    def test_existing_output_is_never_overwritten(self):
        source = self.write('out/source.log', b'old failure\x00\xff')
        target = self.write('docs/verification/archive/old/source.log', b'do not overwrite')
        with self.assertRaises(ValueError):
            common.archive_destination(str(target.parent))
        with self.assertRaises(FileExistsError):
            common.copy_evidence(source, target)
        self.assertEqual(target.read_bytes(), b'do not overwrite')
        self.assertEqual(source.read_bytes(), b'old failure\x00\xff')

    def test_exact_copy_keeps_failure_bytes_and_source(self):
        source = self.write('out/failed.stderr.log', b'failure\r\n\x00\xff')
        target = self.root / 'docs/verification/archive/new/failed.stderr.log'
        copied = common.copy_evidence(source, target)
        self.assertEqual(source.read_bytes(), target.read_bytes())
        self.assertEqual(copied, {'bytes': source.stat().st_size, 'sha256': common.sha256(source)})

    def test_changed_source_keeps_partial_output_and_original_location(self):
        source = self.write('out/source.log', b'first')
        target = self.root / 'docs/verification/archive/new/source.log'
        copy = common.shutil.copyfileobj

        def change(incoming, outgoing):
            copy(incoming, outgoing)
            source.write_bytes(b'changed after copying')

        with patch.object(common.shutil, 'copyfileobj', side_effect=change), self.assertRaises(ValueError):
            common.copy_evidence(source, target)
        self.assertEqual(target.read_bytes(), b'first')
        self.assertTrue(source.exists())

    def test_linked_ancestor_is_rejected(self):
        linked = self.write('docs/verification/archive/link/placeholder').parent
        real = Path.is_symlink
        with patch.object(Path, 'is_symlink', lambda path: path == linked or real(path)), self.assertRaises(ValueError):
            common.archive_destination(linked / 'new')

    def test_all_collectors_reject_existing_output_before_reading_inputs(self):
        output = self.write('docs/verification/archive/existing/keep').parent
        for module, arguments in ((candidate, argparse.Namespace(input=[], output_directory=output)),
                                  (preparation, argparse.Namespace(OutputDirectory=str(output))),
                                  (inputs, argparse.Namespace(OutputDirectory=str(output), AuditDirectory='out/missing'))):
            with self.subTest(module=module.__file__), patch.object(module, 'ROOT', self.root), \
                    patch.object(argparse.ArgumentParser, 'parse_args', return_value=arguments), self.assertRaises(ValueError):
                module.main()

    def test_candidate_archive_records_exact_payload_and_excludes_runtime_package(self):
        source = self.write('out/m3-t3/batch/failed.log', b'original failure')
        self.write('out/m3-t3/batch/result.json', b'{"passed": false}')
        self.write('out/m3-t3/batch/game.exe', b'not a runnable snapshot')
        self.write('CMakeLists.txt')
        self.write('scripts/package-d3d12-candidate.py')
        for directory in ('game', 'cmake', 'test/integration/renderer', 'assets/shaders/hlsl'):
            self.write(directory + '/input.h')
        output = self.root / 'docs/verification/archive/new'
        args = argparse.Namespace(input=[source.parent], output_directory=output)
        with patch.object(candidate, 'ROOT', self.root), patch.object(argparse.ArgumentParser, 'parse_args', return_value=args):
            candidate.main()
        manifest = json.loads((output / 'validation-manifest.json').read_text())
        self.assertFalse(manifest['runnable_package'])
        self.assertFalse(manifest['r3_acceptance'])
        for row in manifest['files']:
            self.assertEqual(common.sha256(output / row['path']), row['sha256'])
        self.assertFalse((output / 'raw/out/m3-t3/batch/game.exe').exists())
        self.assertEqual((output / 'raw/out/m3-t3/batch/failed.log').read_bytes(), source.read_bytes())
        for path in output.rglob('*'):
            if path.is_file():
                path.chmod(0o666)

    def test_candidate_rejects_outside_source_without_creating_archive(self):
        source = self.write('out/other/failed.log')
        output = self.root / 'docs/verification/archive/new'
        args = argparse.Namespace(input=[source.parent], output_directory=output)
        with patch.object(candidate, 'ROOT', self.root), patch.object(argparse.ArgumentParser, 'parse_args', return_value=args), self.assertRaises(SystemExit):
            candidate.main()
        self.assertFalse(output.exists())

    def test_preparation_collector_records_copies_without_removing_inputs(self):
        source = self.root / 'out/m3-t2'
        for name in preparation.COPIES:
            self.write('out/m3-t2/' + name, b'original failed or passed result')
        for name in ('vendor/sdl3/.git-hash', 'vendor/sdl3/REVISION.txt', 'vendor/sdl3/LICENSE.txt',
                     'out/m3-t2/baseline-input-001/build-inputs.json', 'out/m3-t2/build/glfw-debug/bin/SymoCraft.exe',
                     'out/m3-t2/install/glfw-release/SymoCraft.exe', 'out/m3-t2/install/glfw-release/SymoCraftBenchmark.exe',
                     'out/m3-t2/build/glfw-release/game/modules/platform/symocraft_platform.lib',
                     'out/m3-t2/probe/gl-debug/SymoCraftSdl3Probe.exe', 'out/m3-t2/probe/vulkan-debug/SymoCraftSdl3Probe.exe',
                     'out/m3-t2/install/sdl-gl-probe-release/SymoCraftSdl3Probe.exe'):
            self.write(name)
        common.write_json(source / 'sdl-official-vendor-final-comparison.json',
                          {'different': [], 'missing': [], 'extra': [], 'archive_sha256': 'a' * 64, 'archive_files': 1, 'identical': 1})
        destination = self.root / 'docs/verification/archive/preparation/new'
        args = argparse.Namespace(OutputDirectory=str(destination))
        git = argparse.Namespace(stdout='fixture-commit')
        with patch.object(preparation, 'ROOT', self.root), patch.object(preparation, 'source_inventory', return_value=[]), \
                patch.object(preparation.subprocess, 'run', return_value=git), \
                patch.object(argparse.ArgumentParser, 'parse_args', return_value=args):
            preparation.main()
        manifest = json.loads((destination / 'archive-manifest.json').read_text())
        self.assertEqual(len(manifest['copied_sources']), len(preparation.COPIES))
        self.assertFalse(manifest['acceptance'])
        self.assertTrue((source / 'Symocraft-M3-T2-GLFW-baseline-windows-x64.zip').exists())
        for row in manifest['copied_sources']:
            self.assertEqual(common.sha256(self.root / row['source']), row['sha256'])
            self.assertEqual(common.sha256(destination / row['path']), row['sha256'])

    def test_input_collector_records_copies_and_retains_partial_result(self):
        source = self.root / 'out/m3-t2'
        audit = source / 'audit'
        self.write('out/m3-t2/audit/inputs-sha256.json')
        self.write('out/m3-t2/audit/import-audit.json')
        common.write_json(audit / 'summary.json', {'completed': True, 'cases': [{'completed': True, 'changed_inputs': []}] * 4})
        common.write_json(self.write('out/m3-t2/baseline-input-001/build-inputs.json'), {'files': []})
        common.write_json(source / 'sdl-official-vendor-input-v2-comparison.json',
                          {'different': [], 'missing': [], 'extra': [], 'identical': 2183, 'archive_files': 2183, 'archive_sha256': 'a' * 64})
        for name in ('input-comparison/automated-002/result.json', 'probe/gl-release-cmake322-runtime-001/summary.json',
                     'runtime/sdl-input-002/result.json', 'runtime/sdl-input-002/stdout.log', 'runtime/sdl-input-002/stderr.log',
                     'probe/input-comparison-sdl-release-002/Testing/Temporary/LastTest.log',
                     'probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe',
                     'probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe',
                     'probe/input-comparison-sdl-release-002/SymoCraftSdl3RuntimeInputTests.exe',
                     'probe/input-comparison-sdl-release-002/SymoCraftSdl3InputCandidateTests.exe',
                     'input-comparison-sdl-release-002-build.log'):
            self.write('out/m3-t2/' + name, b'partial input result')
        archive = self.write('out/m3-t2/tools/cmake-3.22.6/cmake-3.22.6-windows-x86_64.zip')
        archive.with_name('cmake-3.22.6-SHA-256.txt').write_text(common.sha256(archive) + '  ' + archive.name)
        destination = self.root / 'docs/verification/archive/input/new'
        args = argparse.Namespace(OutputDirectory=str(destination), AuditDirectory=str(audit))
        with patch.object(inputs, 'ROOT', self.root), patch.object(inputs, 'snapshot_identity', return_value=[]), \
                patch.object(argparse.ArgumentParser, 'parse_args', return_value=args):
            inputs.main()
        manifest = json.loads((destination / 'archive-manifest.json').read_text())
        self.assertEqual(len(manifest['copied_sources']), 8)
        self.assertFalse(manifest['acceptance'])
        for row in manifest['copied_sources']:
            self.assertEqual(common.sha256(self.root / row['source']), row['sha256'])
            self.assertEqual(common.sha256(destination / row['path']), row['sha256'])


if __name__ == '__main__':
    unittest.main()
