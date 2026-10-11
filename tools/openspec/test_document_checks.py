"""Small fixtures for link parsing and documentation ownership checks."""

import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
import check_documents as checks


class DocumentChecks(unittest.TestCase):
    def test_code_links_are_not_navigation(self):
        body = '[real](ok.md)\n```md\n[example](missing.md)\n```\n`[inline](missing.md)`\n'
        self.assertEqual([match[1] for match in checks.link_matches(body)], ['ok.md'])

    def test_images_reference_and_angle_paths(self):
        body = '![image](picture.png)\n[title](<folder/my note.md>)\n[ref]: other.md "Title"\n'
        self.assertEqual([match[1] for match in checks.link_matches(body)],
                         ['picture.png', '<folder/my note.md>', 'other.md'])

    def test_unicode_duplicate_and_explicit_anchors(self):
        body = '# 所有权、失败\n# 所有权、失败\n<a id="s-01"></a>\n'
        self.assertEqual(checks.slugs(body), {'所有权失败', '所有权失败-1', 's-01'})

    def test_heading_inline_code_keeps_its_text(self):
        self.assertIn('advance-时间与错误契约', checks.slugs('## `Advance` 时间与错误契约\n'))

    def test_fenced_headings_are_not_anchors(self):
        self.assertEqual(checks.slugs('```md\n# Example\n```\n# Actual\n'), {'actual'})

    def test_ownership_comes_only_from_frontmatter(self):
        self.assertEqual(checks.tags('---\ntags:\n  - area/spec\n  - topic/physics\n---\n'),
                         ['area/spec', 'topic/physics'])
        self.assertEqual(checks.tags('# tags:\n  - area/meta\n'), [])

    def test_local_links_wikis_and_ownership_errors_are_detected(self):
        with tempfile.TemporaryDirectory(prefix='symocraft-doc-check-') as directory:
            root = Path(directory).resolve()
            docs = root / 'docs'
            docs.mkdir()
            a, b = docs / 'a.md', docs / 'b.md'
            a.write_text('---\ntags:\n  - area/meta\n---\n# A\n[b](b.md#b)\n[[b#B]]\n[bad](b.md#missing)\n', encoding='utf-8')
            b.write_text('# B\n', encoding='utf-8')
            with patch.object(checks, 'ROOT', root), patch.object(checks, 'documents', return_value=iter([a, b])):
                result = checks.audit()
            self.assertEqual(result['links'], 2)
            self.assertEqual(result['wikiLinks'], 1)
            self.assertEqual([error['problem'] for error in result['errors']],
                             ['missing anchor', 'expected one area tag'])

    def test_new_output_link_is_not_silently_downgraded(self):
        root = Path(__file__).resolve().parents[2]
        path = root / 'docs/architecture/not-a-real-test-note.md'
        body = '---\ntags:\n  - area/architecture\n---\n[bad](../../out/not-a-real-test-result/missing.json)\n'
        with patch.object(checks.evidence_paths, 'load_relocations', return_value=({}, set())), \
                patch.object(checks, 'documents', return_value=iter([path])), patch.object(Path, 'read_text', return_value=body):
            result = checks.audit()
        self.assertEqual(result['errors'][0]['problem'], 'missing target')
        self.assertEqual(result['warnings'], [])

    def test_direct_archive_markdown_anchors_are_checked(self):
        with tempfile.TemporaryDirectory(prefix='archive-anchor-tests-', dir=Path(__file__).resolve().parents[2] / 'out') as directory:
            root = Path(directory)
            target = root / 'docs/verification/archive/record.md'
            target.parent.mkdir(parents=True)
            target.write_text('# Topic\n', encoding='utf-8')
            source = root / 'docs/current.md'
            source.write_text('---\ntags:\n  - area/verification\n---\n[good](verification/archive/record.md#topic)\n[bad](verification/archive/record.md#missing)\n', encoding='utf-8')
            with patch.object(checks, 'ROOT', root), patch.object(checks, 'documents', return_value=iter([source])):
                result = checks.audit()
            self.assertEqual([item['problem'] for item in result['errors']], ['missing anchor'])

    def test_openspec_ownership_is_specific(self):
        root = Path(__file__).resolve().parents[2]
        path = root / 'docs/openspec/specs/not-a-real-capability/spec.md'
        body = '---\ntags:\n  - area/meta\n---\n# Wrong ownership\n'
        with patch.object(checks.evidence_paths, 'load_relocations', return_value=({}, set())), \
                patch.object(checks, 'documents', return_value=iter([path])), patch.object(Path, 'read_text', return_value=body):
            result = checks.audit()
        self.assertEqual(result['errors'][0]['problem'], 'OpenSpec requires area/spec')

    def test_retirement_is_explicit_and_not_a_fenced_example(self):
        marker = '> 2026-10-11 文档浓缩后已退役复核源：当前见模块。\n'
        self.assertTrue(checks.retired_review_source('project/architecture/old.md', marker))
        self.assertFalse(checks.retired_review_source('architecture/current.md', '# Current\n'))
        self.assertFalse(checks.retired_review_source('architecture/current.md', '```md\n' + marker + '```\n'))

    def test_retired_note_is_still_checked_for_missing_links(self):
        root = Path(__file__).resolve().parents[2]
        path = root / 'docs/project/architecture/not-a-real-review-note.md'
        body = '---\ntags:\n  - area/architecture\n---\n# Old\n> 2026-10-11 文档浓缩后已退役复核源：已迁移。\n[bad](missing.json)\n'
        with patch.object(checks.evidence_paths, 'load_relocations', return_value=({}, set())), \
                patch.object(checks, 'documents', return_value=iter([path])), patch.object(Path, 'read_text', return_value=body):
            result = checks.audit()
        self.assertEqual(result['errors'][0]['problem'], 'missing target')

    def test_index_separates_retired_sources_without_reading_private_body(self):
        with tempfile.TemporaryDirectory(prefix='symocraft-index-') as directory:
            root = Path(directory).resolve()
            docs = root / 'docs'
            docs.mkdir()
            current, retired = docs / 'current.md', docs / 'old.md'
            header = '---\ntags:\n  - area/architecture\n---\n# Note\n'
            current.write_text(header, encoding='utf-8')
            retired.write_text(header + '> 2026-10-11 文档浓缩后已退役复核源：当前见模块。\n', encoding='utf-8')
            private = docs / 'development-journal/private.md'
            private.parent.mkdir()
            private.write_text('This fixture body must not be read.', encoding='utf-8')
            (docs / 'tag-inventory.json').write_text(
                '{"preservedPrivateEntries": [{"path": "development-journal/private.md", "area": "area/journal"}]}',
                encoding='utf-8')
            original_read = Path.read_text

            def guarded_read(path, *args, **kwargs):
                self.assertNotEqual(path, private, 'Private body was read')
                return original_read(path, *args, **kwargs)

            with patch.object(checks, 'ROOT', root), \
                    patch.object(checks, 'documents', return_value=iter([current, retired])), \
                    patch.object(Path, 'read_text', autospec=True, side_effect=guarded_read):
                counts = checks.write_index()
            self.assertEqual(counts['managed'], 1)
            self.assertEqual(counts['retiredReviewSources'], 1)
            self.assertEqual(counts['preservedPrivateRegistrations'], 1)


class HistoricalRelocationChecks(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='symocraft-history-check-')
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.review = self.root / 'docs/spec/review.md'
        self.target = self.root / 'docs/verification/archive/project-history-20261011/old.md'
        self.registry = self.root / checks.RETIREMENT
        self.review.parent.mkdir(parents=True)
        self.target.parent.mkdir(parents=True)
        self.review.write_text('---\ntags:\n  - area/spec\n---\n# Review\n[old](../project/old.md#kept)\n', encoding='utf-8')
        self.target.write_text('# Kept\nOriginal historical bytes.\n', encoding='utf-8')
        self.record = {'schemaVersion': 1,
                       'documents': [{'source': 'docs/project/old.md',
                                      'destination': self.target.relative_to(self.root).as_posix(),
                                      **self.identity(self.target)}],
                       'readOnlyReviewSources': [{'path': self.review.relative_to(self.root).as_posix(),
                                                 **self.identity(self.review)}]}
        self.save()

    def identity(self, path):
        raw = path.read_bytes()
        return {'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}

    def save(self):
        self.registry.write_text(json.dumps(self.record), encoding='utf-8')

    def audit(self, paths=None):
        with patch.object(checks, 'ROOT', self.root), \
                patch.object(checks, 'documents', return_value=iter(paths or [self.review])):
            return checks.audit()

    def update_review(self, body):
        self.review.write_text(body, encoding='utf-8')
        self.record['readOnlyReviewSources'][0].update(self.identity(self.review))
        self.save()

    def test_registered_original_and_anchor_resolve(self):
        result = self.audit()
        self.assertEqual(result['errors'], [])
        self.assertEqual(result['historicalLinks'], 1)
        self.assertFalse((self.root / 'docs/project/old.md').exists())

    def test_missing_or_changed_archive_is_not_accepted(self):
        self.target.write_text('# Kept\nAltered.\n', encoding='utf-8')
        self.assertEqual(self.audit()['errors'][0]['problem'], 'invalid historical relocation registry')
        self.target.unlink()
        self.assertEqual(self.audit()['errors'][0]['problem'], 'invalid historical relocation registry')

    def test_read_only_source_identity_is_required(self):
        self.review.write_text(self.review.read_text(encoding='utf-8') + 'Changed.\n', encoding='utf-8')
        self.assertEqual(self.audit()['errors'][0]['problem'], 'invalid historical relocation registry')

    def test_live_or_unregistered_source_cannot_use_mapping(self):
        for relative in ('docs/architecture/current.md', 'docs/spec/unregistered.md'):
            with self.subTest(relative=relative):
                path = self.root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                area = 'area/spec' if '/spec/' in relative else 'area/architecture'
                path.write_text('---\ntags:\n  - ' + area + '\n---\n[old](../project/old.md#kept)\n', encoding='utf-8')
                result = self.audit([self.review, path])
                self.assertEqual([error['problem'] for error in result['errors']], ['missing target'])

    def test_unknown_target_and_missing_anchor_still_fail(self):
        self.update_review('---\ntags:\n  - area/spec\n---\n[bad](../project/old.md#missing)\n[unknown](../project/unknown.md)\n')
        self.assertEqual([error['problem'] for error in self.audit()['errors']],
                         ['missing anchor', 'missing target'])

    def test_invalid_scope_duplicate_and_identity_fail_closed(self):
        original = copy.deepcopy(self.record)
        invalid = [('source', '../outside.md'), ('source', 'docs/project/../outside.md'),
                   ('destination', 'docs/verification/archive/other/old.md'), ('bytes', True),
                   ('bytes', original['documents'][0]['bytes'] + 1), ('sha256', '0' * 64)]
        for key, value in invalid:
            with self.subTest(key=key, value=value):
                self.record = copy.deepcopy(original)
                self.record['documents'][0][key] = value
                self.save()
                self.assertEqual(self.audit()['errors'][0]['problem'], 'invalid historical relocation registry')
        self.record = copy.deepcopy(original)
        self.record['documents'].append(copy.deepcopy(self.record['documents'][0]))
        self.save()
        self.assertEqual(self.audit()['errors'][0]['problem'], 'invalid historical relocation registry')

    def test_missing_registry_does_not_hide_old_link(self):
        self.registry.unlink()
        self.assertEqual(self.audit()['errors'][0]['problem'], 'missing target')


if __name__ == '__main__':
    unittest.main(verbosity=2)
