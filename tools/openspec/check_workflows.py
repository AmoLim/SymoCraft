"""Exercise the installed CLI in isolated fixtures, then audit project documents."""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from urllib.parse import unquote

sys.dont_write_bytecode = True
from run import PLANNING_ROOT, cli_environment, discover_cli

ROOT = Path(__file__).resolve().parents[2]


def migration_change(planning_root):
    changes = planning_root / 'openspec/changes'
    active = changes / 'docs-openspec-migration'
    candidates = ([active] if active.is_dir() else []) + list(
        (changes / 'archive').glob('????-??-??-docs-openspec-migration'))
    if len(candidates) != 1 or not (candidates[0] / '.openspec.yaml').is_file():
        raise RuntimeError('Expected one active or archived migration change with metadata')
    return candidates[0]


CHANGE = migration_change(PLANNING_ROOT)
parser = argparse.ArgumentParser()
parser.add_argument('--cli-entry', type=Path, help='Installed OpenSpec bin/openspec.js, if global discovery is unavailable')
options = parser.parse_args()


def execute(command, cwd):
    env = cli_environment()
    return subprocess.run(command, cwd=cwd, env=env, capture_output=True, text=True,
                          encoding='utf-8', errors='strict', timeout=60, check=False)


CLI = discover_cli(options.cli_entry)
NODE = shutil.which('node')
if not NODE:
    raise RuntimeError('Node.js unavailable')


def cli(*args, cwd=PLANNING_ROOT, expected=0):
    result = execute([NODE, str(CLI), *args], cwd)
    if result.returncode != expected:
        raise AssertionError(f'{args}: exit {result.returncode}\n{result.stdout}\n{result.stderr}')
    return json.loads(result.stdout)


def put(root, relative, content):
    target = root / relative
    assert target.resolve().is_relative_to(root.resolve())
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding='utf-8', newline='\n')


PROPOSAL = '''## Why
Isolated workflow acceptance fixture.
## What Changes
Add fixture behavior.
## Capabilities
- fixture: test-only capability.
## Impact
No project runtime edits.
## Implementation
- [ ] Implement fixture and verify its behavior.
'''
DELTA = '''## Purpose
This test-only capability exercises schema resolution and requirement validation without changing any production contract.
## ADDED Requirements
### Requirement: Fixture behavior
The fixture SHALL return a deterministic result.
#### Scenario: Valid input
- **WHEN** valid input is supplied
- **THEN** the deterministic result is returned.
'''


class Workflows(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix='symocraft-openspec-')
        self.addCleanup(self.directory.cleanup)
        self.repository = Path(self.directory.name).resolve()
        self.fixture = self.repository / 'docs'
        shutil.copytree(PLANNING_ROOT / 'openspec/schemas', self.fixture / 'openspec/schemas')
        shutil.copyfile(PLANNING_ROOT / 'openspec/config.yaml', self.fixture / 'openspec/config.yaml')
        shutil.copyfile(ROOT / 'AGENTS.md', self.repository / 'AGENTS.md')
        runner = self.repository / 'tools/openspec/run.py'
        runner.parent.mkdir(parents=True)
        shutil.copyfile(ROOT / 'tools/openspec/run.py', runner)

    def project_entry(self, *args, cwd=None, repository=None, expected=0, as_json=True):
        repository = repository or ROOT
        result = execute([sys.executable, str(repository / 'tools/openspec/run.py'),
                          '--cli-entry', str(CLI), *args], cwd or repository)
        self.assertEqual(result.returncode, expected, f'{args}\n{result.stdout}\n{result.stderr}')
        return json.loads(result.stdout) if as_json else result

    def new(self, name, schema=None):
        args = ['new', 'change', name]
        if schema:
            args.extend(['--schema', schema])
        cli(*args, '--json', cwd=self.fixture)
        return f'openspec/changes/{name}/'

    def test_installed_version(self):
        result = execute([NODE, str(CLI), '--version'], ROOT)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.strip(), '1.14.1')

    def test_schema_structure_and_project_resolution(self):
        result = cli('schema', 'validate', '--json')
        self.assertTrue(result['valid'])
        self.assertEqual(len(result['schemas']), 2)
        for name in ('symocraft-lite', 'symocraft-full'):
            resolution = cli('schema', 'which', name, '--json')
            self.assertEqual(resolution['source'], 'project')
            self.assertEqual(Path(resolution['path']).resolve(), PLANNING_ROOT / 'openspec/schemas' / name)

    def test_actual_proposal_instruction_has_preread(self):
        for name in ('symocraft-lite', 'symocraft-full'):
            self.new(name, name)
            result = cli('instructions', 'proposal', '--change', name, '--json', cwd=self.fixture)
            self.assertIn('../AGENTS.md', result['instruction'])
            self.assertIn('Proposal 前的规格预读', result['instruction'])
            self.assertTrue(any('规格预读' in item for item in result['rules']))
            self.assertIn('DOCUMENTATION_MAP.md', result['context'])
            self.assertIn('Planning root is docs', result['context'])
            self.assertTrue((self.fixture / '../AGENTS.md').resolve().is_file())

    def test_lite_default_tracks_proposal_without_parallel_plan(self):
        path = self.new('lite-default')
        status = cli('status', '--change', 'lite-default', '--json', cwd=self.fixture)
        self.assertEqual(status['schemaName'], 'symocraft-lite')
        self.assertEqual({a['id'] for a in status['artifacts']}, {'proposal', 'specs'})
        put(self.fixture, path + 'proposal.md', PROPOSAL)
        put(self.fixture, path + 'specs/fixture/spec.md', DELTA)
        progress = cli('instructions', 'apply', '--change', 'lite-default', '--json', cwd=self.fixture)
        self.assertEqual(progress['progress'], {'total': 1, 'complete': 0, 'remaining': 1})
        self.assertEqual(Path(progress['tasks'][0]['sourcePath']).name, 'proposal.md')
        put(self.fixture, path + 'proposal.md', PROPOSAL.replace('- [ ]', '- [x]'))
        done = cli('instructions', 'apply', '--change', 'lite-default', '--json', cwd=self.fixture)
        self.assertEqual(done['progress']['complete'], 1)
        self.assertEqual(done['state'], 'all_done')
        cli('validate', 'lite-default', '--type', 'change', '--strict', '--json', cwd=self.fixture)
        self.assertFalse((self.fixture / path / 'tasks.md').exists())
        self.assertFalse((self.fixture / path / 'design.md').exists())

    def test_no_behavior_change_skips_specs(self):
        path = self.new('docs-only')
        put(self.fixture, path + '.openspec.yaml', 'schema: symocraft-lite\nskip_specs: true\n')
        put(self.fixture, path + 'proposal.md', PROPOSAL)
        status = cli('status', '--change', 'docs-only', '--json', cwd=self.fixture)
        self.assertEqual(next(a for a in status['artifacts'] if a['id'] == 'specs')['status'], 'skipped')
        cli('validate', 'docs-only', '--type', 'change', '--strict', '--json', cwd=self.fixture)
        progress = cli('instructions', 'apply', '--change', 'docs-only', '--json', cwd=self.fixture)
        self.assertEqual(progress['progress']['remaining'], 1)

    def test_full_dependencies_and_tracking(self):
        path = self.new('full-check', 'symocraft-full')
        initial = cli('status', '--change', 'full-check', '--json', cwd=self.fixture)
        states = {a['id']: a['status'] for a in initial['artifacts']}
        self.assertEqual(states, {'proposal': 'ready', 'specs': 'blocked', 'design': 'blocked', 'tasks': 'blocked'})
        put(self.fixture, path + 'proposal.md', PROPOSAL)
        put(self.fixture, path + 'specs/fixture/spec.md', DELTA)
        middle = cli('status', '--change', 'full-check', '--json', cwd=self.fixture)
        states = {a['id']: a['status'] for a in middle['artifacts']}
        self.assertEqual(states['design'], 'ready')
        self.assertEqual(states['tasks'], 'blocked')
        put(self.fixture, path + 'design.md', '## Technical Approach\nA test-only deterministic function.\n')
        put(self.fixture, path + 'tasks.md', '- [ ] Implement fixture and run its checks.\n')
        progress = cli('instructions', 'apply', '--change', 'full-check', '--json', cwd=self.fixture)
        self.assertEqual(Path(progress['tasks'][0]['sourcePath']).name, 'tasks.md')
        self.assertEqual(progress['progress']['remaining'], 1)

    def test_invalid_schema_is_rejected(self):
        put(self.fixture, 'openspec/schemas/broken/schema.yaml', 'name: broken\nversion: 1\nartifacts: []\n')
        result = cli('schema', 'validate', 'broken', '--json', cwd=self.fixture, expected=1)
        self.assertFalse(result['valid'])

    def test_migration_state_and_native_edit_scope(self):
        result = cli('validate', '--all', '--strict', '--json')
        self.assertEqual(result['summary']['totals']['failed'], 0)
        for name in ('m3-t3-renderer', 'm3-t4-application-simulation', 'm3-t2-publication-fix1'):
            status = cli('status', '--change', name, '--json')
            self.assertEqual(status['schemaName'], 'symocraft-full')
            self.assertEqual([Path(path).resolve() for path in status['actionContext']['allowedEditRoots']],
                             [PLANNING_ROOT])
            progress = cli('instructions', 'apply', '--change', name, '--json')
            self.assertGreater(progress['progress']['remaining'], 0)
            self.assertIn('confirm current authorization for the C++ code root', progress['context'])
        self.assertTrue(list((PLANNING_ROOT / 'openspec/specs').glob('**/spec.md')))

    def test_preserved_acceptance_and_decision_identities(self):
        changes = PLANNING_ROOT / 'openspec/changes'
        for name, acceptance_count, decision_count in (
                ('m3-t3-renderer', 19, 2), ('m3-t4-application-simulation', 27, 13)):
            specs = '\n'.join(path.read_text(encoding='utf-8') for path in (changes / name / 'specs').rglob('*.md'))
            decisions = (changes / name / 'decisions.md').read_text(encoding='utf-8')
            for number in range(1, acceptance_count + 1):
                self.assertRegex(specs, rf'(?<![A-Za-z0-9])A{number:02d}(?![0-9])', name)
            for number in range(1, decision_count + 1):
                self.assertIn(f'D{number:02d}', decisions)
        t3 = (changes / 'm3-t3-renderer/tasks.md').read_text(encoding='utf-8')
        for number in range(8, 16):
            self.assertIn(f'T3-M{number:02d}', t3)
        t4 = (changes / 'm3-t4-application-simulation/decisions.md').read_text(encoding='utf-8')
        for number in range(1, 5):
            self.assertIn(f'Q{number:02d}', t4)
        fix = (changes / 'm3-t2-publication-fix1/tasks.md').read_text(encoding='utf-8')
        for number in range(1, 8):
            self.assertIn(f'V{number:02d}', fix)

    def test_nested_root_discovery_and_entry(self):
        self.assertFalse((ROOT / 'openspec').exists())
        self.assertFalse((PLANNING_ROOT / '.agents').exists())
        missing = cli('list', '--json', cwd=ROOT, expected=1)
        self.assertIsNone(missing['root'])
        expected_changes = {'m3-t3-renderer', 'm3-t4-application-simulation', 'm3-t2-publication-fix1'}
        if CHANGE.parent == PLANNING_ROOT / 'openspec/changes':
            expected_changes.add('docs-openspec-migration')
        for cwd in (ROOT, PLANNING_ROOT, CHANGE):
            via_entry = self.project_entry('list', '--json', cwd=cwd)
            self.assertEqual(Path(via_entry['root']['path']).resolve(), PLANNING_ROOT)
            self.assertEqual({change['name'] for change in via_entry['changes']},
                             expected_changes)
        nearest = cli('list', '--json', cwd=CHANGE)
        self.assertEqual(Path(nearest['root']['path']).resolve(), PLANNING_ROOT)

    def test_migration_locator_accepts_active_and_archived(self):
        active = 'openspec/changes/docs-openspec-migration/'
        put(self.fixture, active + '.openspec.yaml', 'schema: symocraft-full\n')
        self.assertEqual(migration_change(self.fixture), self.fixture / active)
        target = self.fixture / 'openspec/changes/archive/2026-10-11-docs-openspec-migration'
        target.parent.mkdir()
        (self.fixture / active).rename(target)
        self.assertEqual(migration_change(self.fixture), target)

    def test_migration_locator_rejects_missing_and_duplicate(self):
        with self.assertRaises(RuntimeError):
            migration_change(self.fixture)
        put(self.fixture, 'openspec/changes/docs-openspec-migration/.openspec.yaml', 'schema: symocraft-full\n')
        put(self.fixture, 'openspec/changes/archive/2026-10-11-docs-openspec-migration/.openspec.yaml', 'schema: symocraft-full\n')
        with self.assertRaises(RuntimeError):
            migration_change(self.fixture)

    def test_project_entry_rejects_implicit_and_duplicate_roots(self):
        self.project_entry('init', repository=self.repository, expected=1, as_json=False)
        (self.repository / 'openspec').mkdir()
        result = self.project_entry('list', '--json', repository=self.repository, expected=1, as_json=False)
        self.assertIn('Duplicate root', result.stderr)
        self.assertFalse((self.repository / 'openspec/config.yaml').exists())

    def test_entry_lifecycle_and_archive_in_isolation(self):
        self.project_entry('new', 'change', 'nested-lifecycle', '--json', repository=self.repository)
        path = 'openspec/changes/nested-lifecycle/'
        put(self.fixture, path + 'proposal.md', PROPOSAL.replace('- [ ]', '- [x]'))
        put(self.fixture, path + 'specs/fixture/spec.md', DELTA)
        self.project_entry('validate', 'nested-lifecycle', '--type', 'change', '--strict', '--json',
                           repository=self.repository)
        progress = self.project_entry('instructions', 'apply', '--change', 'nested-lifecycle', '--json',
                                      repository=self.repository)
        self.assertEqual(progress['state'], 'all_done')
        self.project_entry('archive', 'nested-lifecycle', '--yes', '--json', repository=self.repository)
        self.assertTrue((self.fixture / 'openspec/specs/fixture/spec.md').is_file())
        self.assertFalse((self.fixture / path).exists())
        self.assertEqual(len(list((self.fixture / 'openspec/changes/archive').glob('*-nested-lifecycle'))), 1)
        self.project_entry('validate', '--specs', '--strict', '--json', repository=self.repository)
        self.assertFalse((self.repository / 'openspec').exists())

    def test_official_update_keeps_skills_at_code_root(self):
        from unittest.mock import patch

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        put(self.repository, '.agents/skills/local-helper/SKILL.md', 'Unrelated skill; preserve exactly.\n')
        config_home = self.repository / 'user-config'
        put(config_home, 'openspec/config.json', '{"profile":"core","delivery":"skills","featureFlags":{}}\n')
        before = {path.relative_to(self.repository): path.read_bytes()
                  for path in self.repository.rglob('*') if path.is_file()}
        with patch.dict(os.environ, {'XDG_CONFIG_HOME': str(config_home)}):
            self.project_entry('update', '--force', repository=self.repository, as_json=False)
        after = {path.relative_to(self.repository): path.read_bytes()
                 for path in self.repository.rglob('*') if path.is_file()}
        self.assertEqual(before, after)
        self.assertFalse((self.fixture / '.agents').exists())
        self.assertFalse((self.repository / 'openspec').exists())
        self.project_entry('update', 'docs', repository=self.repository, expected=1, as_json=False)

    def test_update_preserves_concurrent_skill_edits(self):
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        source = self.repository / '.agents/skills/openspec-propose/SKILL.md'
        with patch.object(run, 'ROOT', self.repository), patch.object(run, 'PLANNING_ROOT', self.fixture):
            def concurrent_edit(*args, **kwargs):
                source.write_bytes(b'Concurrent user edit\n')
                return subprocess.CompletedProcess(args[0], 0)

            with patch.object(run.subprocess, 'run', side_effect=concurrent_edit):
                with self.assertRaisesRegex(RuntimeError, 'concurrent edits'):
                    run.refresh_skills([NODE, str(CLI)], [])
        self.assertEqual(source.read_bytes(), b'Concurrent user edit\n')

    def test_update_rejects_edits_while_generated_bytes_are_read(self):
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        relative = Path('openspec-propose/SKILL.md')
        source = self.repository / '.agents/skills' / relative
        generated = None
        edited = False
        read_bytes = Path.read_bytes

        def official_update(command, cwd, **kwargs):
            nonlocal generated
            generated = cwd / '.agents/skills' / relative
            generated.write_bytes(b'Official generated update\n')
            return subprocess.CompletedProcess(command, 0)

        def read_generated(path):
            nonlocal edited
            content = read_bytes(path)
            if path == generated and not edited:
                source.write_bytes(b'Concurrent user edit during generation read\n')
                edited = True
            return content

        with patch.object(run, 'ROOT', self.repository), patch.object(run, 'PLANNING_ROOT', self.fixture):
            with patch.dict(os.environ, {'XDG_CONFIG_HOME': str(self.repository / 'user-config')}), \
                    patch.object(run.subprocess, 'run', side_effect=official_update), \
                    patch.object(Path, 'read_bytes', read_generated):
                with self.assertRaisesRegex(RuntimeError, 'concurrent edits'):
                    run.refresh_skills([NODE, str(CLI)], [])
        self.assertTrue(edited)
        self.assertEqual(source.read_bytes(), b'Concurrent user edit during generation read\n')

    def test_update_rechecks_inputs_between_publications(self):
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        skills = self.repository / '.agents/skills'
        relative = [path for path in run.skill_files(skills) if path.name == 'SKILL.md'][:2]
        self.assertEqual(len(relative), 2)
        first, second = [skills / path for path in relative]
        published = False
        write_bytes = Path.write_bytes
        official = [b'First official update\n', b'Second official update\n']

        def official_update(command, cwd, **kwargs):
            for path, content in zip(relative, official):
                write_bytes(cwd / '.agents/skills' / path, content)
            return subprocess.CompletedProcess(command, 0)

        def publish(path, content):
            nonlocal published
            count = write_bytes(path, content)
            if path == first:
                write_bytes(second, b'Concurrent user edit between publications\n')
                published = True
            return count

        with patch.object(run, 'ROOT', self.repository), patch.object(run, 'PLANNING_ROOT', self.fixture):
            with patch.dict(os.environ, {'XDG_CONFIG_HOME': str(self.repository / 'user-config')}), \
                    patch.object(run.subprocess, 'run', side_effect=official_update), \
                    patch.object(Path, 'write_bytes', publish):
                with self.assertRaisesRegex(RuntimeError, 'concurrent edits'):
                    run.refresh_skills([NODE, str(CLI)], [])
        self.assertTrue(published)
        self.assertEqual(first.read_bytes(), official[0])
        self.assertEqual(second.read_bytes(), b'Concurrent user edit between publications\n')

    def test_managed_skill_symbolic_links_are_rejected(self):
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        put(self.repository, '.agents/skills/openspec-propose/references/fixture.txt', 'Fixture input\n')
        put(self.repository, '.agents/skills/.openspec-target', 'Fixture marker\n')
        skills = self.repository / '.agents/skills'
        paths = [skills.parent, skills, skills / 'openspec-propose',
                 skills / 'openspec-propose/SKILL.md', skills / 'openspec-propose/references',
                 skills / 'openspec-propose/references/fixture.txt', skills / '.openspec-target']
        is_symlink = Path.is_symlink
        for linked in paths:
            with self.subTest(path=linked.relative_to(self.repository).as_posix()):
                with patch.object(Path, 'is_symlink', lambda path: path == linked or is_symlink(path)):
                    with self.assertRaisesRegex(RuntimeError, 'linked|Unexpected skill path'):
                        run.skill_files(skills)

    def test_managed_skill_reparse_points_are_rejected_without_junction_api(self):
        from types import SimpleNamespace
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        put(self.repository, '.agents/skills/openspec-propose/references/fixture.txt', 'Fixture input\n')
        put(self.repository, '.agents/skills/.openspec-target', 'Fixture marker\n')
        skills = self.repository / '.agents/skills'
        paths = [skills.parent, skills, skills / 'openspec-propose',
                 skills / 'openspec-propose/SKILL.md', skills / 'openspec-propose/references',
                 skills / 'openspec-propose/references/fixture.txt', skills / '.openspec-target']
        lstat = Path.lstat
        is_symlink = Path.is_symlink
        for linked in paths:
            def reparse_stat(path):
                info = lstat(path)
                if path != linked:
                    return info
                values = {name: getattr(info, name) for name in dir(info) if name.startswith('st_')}
                values['st_file_attributes'] = values.get('st_file_attributes', 0) | 0x400
                return SimpleNamespace(**values)

            with self.subTest(path=linked.relative_to(self.repository).as_posix()):
                with patch.object(Path, 'is_symlink', lambda path: False if path == linked else is_symlink(path)), \
                        patch.object(Path, 'lstat', reparse_stat), \
                        patch.object(Path, 'is_junction', side_effect=AssertionError('Junction API must not be required'),
                                     create=True):
                    with self.assertRaisesRegex(RuntimeError, 'linked|Unexpected skill path'):
                        run.skill_files(skills)

    def test_update_file_set_change_needs_review(self):
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        before = {path.relative_to(self.repository): path.read_bytes()
                  for path in (self.repository / '.agents').rglob('*') if path.is_file()}
        with patch.object(run, 'ROOT', self.repository), patch.object(run, 'PLANNING_ROOT', self.fixture):
            def add_workflow(command, cwd, **kwargs):
                self.assertTrue(Path(kwargs['env']['XDG_CONFIG_HOME']).is_relative_to(cwd))
                put(cwd, '.agents/skills/openspec-new-workflow/SKILL.md', 'New generated workflow\n')
                return subprocess.CompletedProcess(command, 0)

            with patch.object(run.subprocess, 'run', side_effect=add_workflow):
                with self.assertRaisesRegex(RuntimeError, 'adds/removes'):
                    run.refresh_skills([NODE, str(CLI)], [])
        after = {path.relative_to(self.repository): path.read_bytes()
                 for path in (self.repository / '.agents').rglob('*') if path.is_file()}
        self.assertEqual(before, after)

    def test_update_removed_file_needs_review(self):
        from unittest.mock import patch
        import run

        shutil.copytree(ROOT / '.agents/skills', self.repository / '.agents/skills')
        source = self.repository / '.agents/skills/openspec-propose/SKILL.md'
        before = source.read_bytes()
        with patch.object(run, 'ROOT', self.repository), patch.object(run, 'PLANNING_ROOT', self.fixture):
            def remove_workflow(command, cwd, **kwargs):
                (cwd / '.agents/skills/openspec-propose/SKILL.md').unlink()
                return subprocess.CompletedProcess(command, 0)

            with patch.object(run.subprocess, 'run', side_effect=remove_workflow):
                with self.assertRaisesRegex(RuntimeError, 'adds/removes'):
                    run.refresh_skills([NODE, str(CLI)], [])
        self.assertEqual(source.read_bytes(), before)

    def test_current_document_links(self):
        import evidence_paths

        files = [ROOT / 'AGENTS.md', ROOT / 'README.md', ROOT / 'docs/DOCUMENTATION_MAP.md']
        files += list((ROOT / 'docs/scope').glob('*.md'))
        files += list((ROOT / 'docs/verification').glob('*.md'))
        changes = PLANNING_ROOT / 'openspec/changes'
        files += [path for path in changes.rglob('*.md') if path.relative_to(changes).parts[0] != 'archive']
        files += list((PLANNING_ROOT / 'openspec/specs').rglob('*.md'))
        files += [ROOT / 'docs/README.md']
        relocations = {}
        if CHANGE.parent.name == 'archive':
            registry = json.loads((ROOT / evidence_paths.REGISTRY).read_text(encoding='utf-8'))
            relocations = evidence_paths.change_archive_relocations(ROOT, registry)
            self.assertEqual(set(relocations.values()), set(CHANGE.iterdir()))
            files += [path for path in relocations.values() if path.suffix == '.md']
        original_sources = {target: source for source, target in relocations.items()}
        for source in files:
            text = source.read_text(encoding='utf-8')
            for target in re.findall(r'\[[^\]\n]*\]\(([^)\n]+)\)', text):
                if re.match(r'^[a-z][a-z0-9+.-]*:', target, re.I):
                    continue
                target = target.strip('<>')
                path, _, anchor = target.partition('#')
                base = original_sources.get(source, source).parent
                resolved = (base / unquote(path)).resolve() if path else source
                if not resolved.exists() and source in original_sources and resolved in relocations:
                    resolved = relocations[resolved]
                self.assertTrue(resolved.is_relative_to(ROOT), f'Out-of-project link: {source}: {target}')
                self.assertTrue(resolved.exists(), f'Broken link: {source}: {target}')
                if anchor.startswith(('s-', 'sp-v')):
                    self.assertIn(f'id="{anchor}"', resolved.read_text(encoding='utf-8'))


if __name__ == '__main__':
    unittest.main(argv=['check_workflows'], verbosity=2)
