"""Resolve only registered evidence relocations, with exact byte identities."""
import hashlib
import json
from pathlib import Path, PurePosixPath, PureWindowsPath
import re

REGISTRY = 'docs/verification/archive/evidence-consolidation-20261011.json'
NEW_ARCHIVE = 'docs/verification/archive/milestone-evidence-20261011/'
MILESTONES = {'m0', 'm1', 'm2-a', 'm2-t2', 'm2-t3', 'm3-t0', 'm3-t1', 'm3-t2', 'm3-t3', 'm3-t4'}
MIGRATION_CHANGE = 'docs/openspec/changes/docs-openspec-migration'
MIGRATION_ARTIFACTS = {'.openspec.yaml', 'proposal.md', 'design.md', 'tasks.md'}


def registered_path(root, value, prefix):
    if not isinstance(value, str) or not value.startswith(prefix) or '\\' in value or ':' in value:
        raise ValueError('Invalid registered path')
    parts = PurePosixPath(value).parts
    if '..' in parts or PurePosixPath(value).as_posix() != value:
        raise ValueError('Noncanonical registered path')
    path = root / value
    for parent in (path, *path.parents):
        if parent == root.parent:
            break
        if parent.is_symlink() or parent.exists() and getattr(parent.lstat(), 'st_file_attributes', 0) & 0x400:
            raise ValueError('Registered path uses a link/reparse point')
    if path.resolve() != path.absolute() or not path.resolve().is_relative_to(root.resolve()):
        raise ValueError('Registered path escaped its original location')
    return path


def verify_file(path, item):
    if type(item.get('bytes')) is not int or item['bytes'] < 0 or not isinstance(item.get('sha256'), str) or not re.fullmatch('[0-9a-f]{64}', item['sha256']):
        raise ValueError('Invalid evidence file identity')
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    if path.stat().st_size != item['bytes'] or digest.hexdigest() != item['sha256']:
        raise ValueError('Evidence size or SHA256 mismatch: ' + str(path))


def change_archive_relocations(root, data):
    record = data.get('changeArchiveRelocation')
    if record is None:
        return {}
    if not isinstance(record, dict):
        raise ValueError('Invalid migration change archive record')
    destination = record.get('destinationRoot')
    if record.get('sourceRoot') != MIGRATION_CHANGE or not isinstance(destination, str) or not re.fullmatch(
            r'docs/openspec/changes/archive/\d{4}-\d{2}-\d{2}-docs-openspec-migration', destination):
        raise ValueError('Invalid migration change archive scope')
    source_root = registered_path(root, MIGRATION_CHANGE, 'docs/openspec/changes/')
    archive_root = registered_path(root, destination, 'docs/openspec/changes/archive/')
    if source_root.exists() or not isinstance(record.get('files'), list):
        raise ValueError('Migration change is not uniquely archived')
    targets, names = {}, set()
    for item in record['files']:
        if not isinstance(item, dict):
            raise ValueError('Invalid archived migration artifact record')
        name = item.get('name')
        if name not in MIGRATION_ARTIFACTS or name in names or item.get('source') != MIGRATION_CHANGE + '/' + name or item.get('target') != destination + '/' + name:
            raise ValueError('Invalid or duplicate archived migration artifact')
        names.add(name)
        source = registered_path(root, item['source'], MIGRATION_CHANGE + '/')
        target = registered_path(root, item['target'], destination + '/')
        verify_file(target, item)
        targets[source] = target
    if names != MIGRATION_ARTIFACTS:
        raise ValueError('Incomplete archived migration artifacts')
    actual = set()
    for path in archive_root.rglob('*'):
        registered_path(root, path.relative_to(root).as_posix(), destination + '/')
        if path.is_file():
            actual.add(path.relative_to(archive_root).as_posix())
    if actual != MIGRATION_ARTIFACTS:
        raise ValueError('Archived migration file set differs from registered artifacts')
    return targets


def load_relocations(root):
    root = Path(root).absolute()
    registry = registered_path(root, REGISTRY, 'docs/verification/archive/')
    if not registry.exists():
        return {}, set()
    data = json.loads(registry.read_text(encoding='utf-8'))
    if type(data.get('schemaVersion')) is not int or data['schemaVersion'] != 1 or not isinstance(data.get('files'), list) or not data['files'] or not isinstance(data.get('readOnlyReviewSources'), list):
        raise ValueError('Invalid evidence relocation registry schema')
    historical = json.loads((root / 'docs/verification/archive/migration-manifest.json').read_text(encoding='utf-8'))
    previous = {row['source']: row for row in historical['files']}
    targets, expected_new = {}, set()
    for item in data['files']:
        name = item['source']
        source = registered_path(root, name, 'docs/')
        parts = PurePosixPath(name).parts
        if not (len(parts) > 4 and parts[:2] == ('docs', 'milestones') and parts[2] in MILESTONES and parts[3] == 'evidence') and not name.startswith('docs/project/benchmark/evidence/'):
            raise ValueError('Unregistered evidence source scope')
        target = registered_path(root, item['target'], 'docs/verification/archive/')
        if source in targets:
            raise ValueError('Duplicate evidence source')
        if name in previous:
            old = previous[name]
            if item['target'] != 'docs/verification/archive/' + old['path'] or (item['bytes'], item['sha256']) != (old['bytes'], old['sha256']):
                raise ValueError('Historical mapping identity changed')
        else:
            if item['target'] != NEW_ARCHIVE + '/'.join(parts[2:]):
                raise ValueError('Inconsistent new evidence destination')
            expected_new.add(target)
        if not isinstance(data.get('originalSourceRoot'), str) or item.get('sourceBase') != str(PureWindowsPath(data['originalSourceRoot']) / PureWindowsPath(name).parent):
            raise ValueError('Invalid point-in-time source base')
        verify_file(target, item)
        targets[source] = target
    new_root = root / NEW_ARCHIVE
    actual_new = set()
    if new_root.exists():
        registered_path(root, NEW_ARCHIVE.rstrip('/'), 'docs/verification/archive/')
        for path in new_root.rglob('*'):
            registered_path(root, path.relative_to(root).as_posix(), NEW_ARCHIVE)
            if path.is_file():
                actual_new.add(path)
    if actual_new != expected_new:
        raise ValueError('New archive file set differs from registered inventory')
    reviews = set()
    for item in data['readOnlyReviewSources']:
        source = registered_path(root, item['path'], 'docs/spec/')
        if source.suffix != '.md' or source in reviews:
            raise ValueError('Invalid or duplicate read-only review identity')
        verify_file(source, item)
        reviews.add(source)
    targets.update(change_archive_relocations(root, data))
    return targets, reviews
