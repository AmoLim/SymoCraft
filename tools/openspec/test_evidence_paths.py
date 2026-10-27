"""Strict evidence relocation registry fixtures; no broad historical fallback."""
import copy
import hashlib
import json
from pathlib import Path, PureWindowsPath
import tempfile
import unittest
import subprocess
import warnings
import zipfile
from unittest.mock import patch

import evidence_paths as paths
import check_documents as checks


class EvidencePaths(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='evidence-path-tests-', dir=Path(__file__).resolve().parents[2] / 'out')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = 'docs/milestones/m0/evidence/failed.log'
        self.target = paths.NEW_ARCHIVE + 'm0/evidence/failed.log'
        self.write(self.target, b'original failure\r\n')
        self.write('docs/spec/review.md', b'# Review\n')
        self.write('docs/verification/archive/migration-manifest.json', b'{"files": []}')
        self.data = {'schemaVersion': 1, 'originalSourceRoot': str(self.root),
                     'files': [{'source': self.source, 'target': self.target,
                                'sourceBase': str(PureWindowsPath(str(self.root)) / PureWindowsPath(self.source).parent),
                                **self.identity(self.target)}],
                     'readOnlyReviewSources': [{'path': 'docs/spec/review.md', **self.identity('docs/spec/review.md')}]}
        self.record()

    def write(self, name, raw):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(raw)

    def identity(self, name):
        raw = (self.root / name).read_bytes()
        return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}

    def record(self):
        self.write(paths.REGISTRY, json.dumps(self.data).encode())

    def test_exact_mapping_and_read_only_source(self):
        targets, sources = paths.load_relocations(self.root)
        self.assertEqual(targets, {self.root / self.source: self.root / self.target})
        self.assertEqual(sources, {self.root / 'docs/spec/review.md'})
        self.assertNotIn(self.root / 'docs/milestones/m0/evidence/unknown.log', targets)

    def test_missing_registry_does_not_create_fallback(self):
        (self.root / paths.REGISTRY).unlink()
        self.assertEqual(paths.load_relocations(self.root), ({}, set()))

    def test_missing_target_and_changed_bytes_fail(self):
        (self.root / self.target).unlink()
        with self.assertRaises(OSError):
            paths.load_relocations(self.root)
        self.write(self.target, b'edited')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def test_review_identity_drift_fails(self):
        self.write('docs/spec/review.md', b'new specification')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def test_invalid_scope_traversal_base_identity_and_types_fail(self):
        original = copy.deepcopy(self.data)
        for key, value in (('source', 'game/source.cpp'), ('source', 'docs/milestones/m0/evidence/../secret'),
                           ('target', 'docs/verification/archive/../outside'), ('target', paths.NEW_ARCHIVE + 'wrong.log'),
                           ('sourceBase', 'other'), ('bytes', True), ('sha256', '0' * 64)):
            with self.subTest(key=key, value=value):
                self.data = copy.deepcopy(original)
                self.data['files'][0][key] = value
                self.record()
                with self.assertRaises((ValueError, OSError)):
                    paths.load_relocations(self.root)

    def test_duplicate_and_extra_archive_file_fail(self):
        self.data['files'].append(copy.deepcopy(self.data['files'][0]))
        self.record()
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)
        self.data['files'].pop()
        self.record()
        self.write(paths.NEW_ARCHIVE + 'm0/evidence/unregistered.log', b'unknown')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def test_registered_review_links_work_but_current_and_unknown_links_fail(self):
        review = self.root / 'docs/spec/review.md'
        body = '---\ntags:\n  - area/spec\n---\n[old](../milestones/m0/evidence/failed.log)\n'
        review.write_text(body, encoding='utf-8')
        self.data['readOnlyReviewSources'][0].update(self.identity('docs/spec/review.md'))
        self.record()
        with patch.object(checks, 'ROOT', self.root), patch.object(checks, 'documents', return_value=iter([review])):
            result = checks.audit()
        self.assertEqual(result['errors'], [])
        self.assertEqual(result['historicalLinks'], 1)
        current = self.root / 'docs/current.md'
        current.write_text(body.replace('area/spec', 'area/verification').replace('../milestones', 'milestones'), encoding='utf-8')
        with patch.object(checks, 'ROOT', self.root), patch.object(checks, 'documents', return_value=iter([current])):
            self.assertEqual(checks.audit()['errors'][0]['problem'], 'missing target')
        review.write_text(body.replace('failed.log', 'unknown.log'), encoding='utf-8')
        self.data['readOnlyReviewSources'][0].update(self.identity('docs/spec/review.md'))
        self.record()
        with patch.object(checks, 'ROOT', self.root), patch.object(checks, 'documents', return_value=iter([review])):
            self.assertEqual(checks.audit()['errors'][0]['problem'], 'missing target')

    def test_linked_target_ancestor_fails(self):
        linked = (self.root / self.target).parent
        actual = Path.is_symlink
        with patch.object(Path, 'is_symlink', lambda path: path == linked or actual(path)), self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def pack(self, members=None):
        target = self.root / self.target
        raw = target.read_bytes() if target.exists() else b'original failure\r\n'
        if target.exists():
            target.unlink()
        archive = self.root / (paths.NEW_ARCHIVE.rstrip('/') + '.7z')
        with zipfile.ZipFile(archive, 'w') as package:
            for name, value in members or [(self.target.split('archive/', 1)[1], raw)]:
                package.writestr(name, value)
        # Zip keeps fixtures stdlib-only; tar detects the container by its bytes.
        return archive

    def test_packed_mapping_requires_exact_member_bytes(self):
        archive = self.pack()
        (self.root / self.target).parent.rmdir()
        # Remove empty loose parents: a partial loose tree is not a packed archive.
        (self.root / self.target).parents[1].rmdir()
        (self.root / paths.NEW_ARCHIVE).rmdir()
        targets, reviews = paths.load_relocations(self.root)
        self.assertEqual(targets[self.root / self.source], archive)
        self.assertEqual(reviews, {self.root / 'docs/spec/review.md'})

    def test_packed_missing_changed_extra_and_unsafe_members_fail(self):
        member = self.target.split('archive/', 1)[1]
        self.pack()
        for parent in [(self.root / self.target).parent, (self.root / self.target).parents[1],
                       self.root / paths.NEW_ARCHIVE]:
            parent.rmdir()
        cases = [[(member, b'changed')], [(member + '.wrong', b'original failure\r\n')],
                 [(member, b'original failure\r\n'), (paths.NEW_ARCHIVE.split('archive/', 1)[1] + 'extra', b'unknown')],
                 [('../outside', b'unknown')], [(member, b'original failure\r\n'), (member, b'duplicate')]]
        for members in cases:
            with self.subTest(members=members), warnings.catch_warnings():
                warnings.simplefilter('ignore', UserWarning)
                self.pack(members)
                with self.assertRaises(ValueError):
                    paths.load_relocations(self.root)

    def test_packed_markdown_keeps_historical_anchor_checks(self):
        (self.root / self.target).unlink()
        self.source = self.source.replace('.log', '.md')
        self.target = self.target.replace('.log', '.md')
        self.write(self.target, b'# Topic\n')
        self.data['files'][0].update(source=self.source, target=self.target, **self.identity(self.target))
        self.pack()
        for parent in [(self.root / self.target).parent, (self.root / self.target).parents[1],
                       self.root / paths.NEW_ARCHIVE]:
            parent.rmdir()
        review = self.root / 'docs/spec/review.md'
        for anchor, expected in (('topic', []), ('missing', ['missing anchor'])):
            review.write_text('---\ntags:\n  - area/spec\n---\n[old](../milestones/m0/evidence/failed.md#' + anchor + ')\n', encoding='utf-8')
            self.data['readOnlyReviewSources'][0].update(self.identity('docs/spec/review.md'))
            self.record()
            with patch.object(checks, 'ROOT', self.root), patch.object(checks, 'documents', return_value=iter([review])):
                result = checks.audit()
            self.assertEqual([row['problem'] for row in result['errors']], expected)
            self.assertEqual(result['historicalLinks'], 1)

    def test_packed_symlink_and_reparse_archive_are_rejected(self):
        member = self.target.split('archive/', 1)[1]
        link = zipfile.ZipInfo(paths.NEW_ARCHIVE.split('archive/', 1)[1] + 'link')
        link.create_system = 3
        link.external_attr = 0o120777 << 16
        archive = self.pack([(member, b'original failure\r\n'), (link, b'outside')])
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)
        self.pack()
        actual = Path.is_symlink
        with patch.object(Path, 'is_symlink', lambda path: path == archive or actual(path)), self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def test_invalid_container_and_missing_tar_fail_closed(self):
        archive = self.pack()
        archive.write_bytes(b'not an archive')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)
        with patch.object(paths.shutil, 'which', return_value=None), self.assertRaises(OSError):
            paths.load_relocations(self.root)

    def test_missing_review_is_verified_from_exact_retirement_commit(self):
        review = self.root / 'docs/spec/review.md'
        raw = review.read_bytes()
        review.unlink()
        with patch.object(paths.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, raw, b'')) as run:
            targets, reviews = paths.load_relocations(self.root)
        self.assertEqual(reviews, set())
        self.assertEqual(targets[self.root / self.source], self.root / self.target)
        self.assertEqual(run.call_args.args[0][-1], paths.RETIRED_REVIEW_COMMIT + ':docs/spec/review.md')
        for status, payload in ((1, b''), (0, b'changed')):
            with patch.object(paths.subprocess, 'run', return_value=subprocess.CompletedProcess([], status, payload, b'')), \
                    self.assertRaises(ValueError):
                paths.load_relocations(self.root)
        self.data['readOnlyReviewSources'].append(copy.deepcopy(self.data['readOnlyReviewSources'][0]))
        self.record()
        with patch.object(paths.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0, raw, b'')), \
                self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def register_change_archive(self):
        destination = 'docs/openspec/changes/archive/2026-10-11-docs-openspec-migration'
        rows = []
        for name in sorted(paths.MIGRATION_ARTIFACTS):
            target = destination + '/' + name
            self.write(target, b'---\ntags:\n  - area/spec\n---\n# Original artifact\n')
            rows.append({'name': name, 'source': paths.MIGRATION_CHANGE + '/' + name,
                         'target': target, **self.identity(target)})
        self.data['changeArchiveRelocation'] = {'sourceRoot': paths.MIGRATION_CHANGE,
                                               'destinationRoot': destination, 'files': rows}
        self.record()
        return destination

    def test_registered_change_archive_preserves_review_links_only(self):
        destination = self.register_change_archive()
        review = self.root / 'docs/spec/review.md'
        body = '---\ntags:\n  - area/spec\n---\n[old](../openspec/changes/docs-openspec-migration/design.md#original-artifact)\n'
        review.write_text(body, encoding='utf-8')
        self.data['readOnlyReviewSources'][0].update(self.identity('docs/spec/review.md'))
        self.record()
        with patch.object(checks, 'ROOT', self.root), patch.object(checks, 'documents', return_value=iter([review])):
            result = checks.audit()
        self.assertEqual(result['errors'], [])
        self.assertEqual(result['historicalLinks'], 1)
        current = self.root / 'docs/current.md'
        current.write_text(body.replace('../openspec', 'openspec'), encoding='utf-8')
        with patch.object(checks, 'ROOT', self.root), patch.object(checks, 'documents', return_value=iter([current])):
            self.assertEqual(checks.audit()['errors'][0]['problem'], 'missing target')
        self.write(destination + '/design.md', b'changed')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)

    def test_change_archive_rejects_extra_missing_duplicate_and_live_sources(self):
        destination = self.register_change_archive()
        original = copy.deepcopy(self.data)
        for kind in ('missing', 'duplicate', 'scope', 'record-type', 'item-type'):
            with self.subTest(kind=kind):
                self.data = copy.deepcopy(original)
                record = self.data['changeArchiveRelocation']
                if kind == 'missing':
                    record['files'].pop()
                elif kind == 'duplicate':
                    record['files'].append(copy.deepcopy(record['files'][0]))
                elif kind == 'scope':
                    record['sourceRoot'] = 'docs/openspec/changes/m3-t3-renderer'
                elif kind == 'record-type':
                    self.data['changeArchiveRelocation'] = []
                else:
                    record['files'][0] = None
                self.record()
                with self.assertRaises(ValueError):
                    paths.load_relocations(self.root)
        self.data = original
        self.record()
        self.write(destination + '/extra.md', b'unknown')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)
        (self.root / destination / 'extra.md').unlink()
        self.write(paths.MIGRATION_CHANGE + '/proposal.md', b'active duplicate')
        with self.assertRaises(ValueError):
            paths.load_relocations(self.root)


if __name__ == '__main__':
    unittest.main()
