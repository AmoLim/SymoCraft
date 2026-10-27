"""Resolve only registered evidence relocations, with exact byte identities."""
import hashlib
import json
from pathlib import Path, PurePosixPath, PureWindowsPath
import re
import shutil
import subprocess
import tarfile
import tempfile

REGISTRY = 'docs/verification/archive/evidence-consolidation-20261011.json'
NEW_ARCHIVE = 'docs/verification/archive/milestone-evidence-20261011/'
MILESTONES = {'m0', 'm1', 'm2-a', 'm2-t2', 'm2-t3', 'm3-t0', 'm3-t1', 'm3-t2', 'm3-t3', 'm3-t4'}
MIGRATION_CHANGE = 'docs/openspec/changes/docs-openspec-migration'
MIGRATION_ARTIFACTS = {'.openspec.yaml', 'proposal.md', 'design.md', 'tasks.md'}
PACKED_ARCHIVES = {'milestone-evidence-20261011', 'benchmark-20261005',
                   't2-approval-20261009', 't3-r1', 't3-r2'}
RETIRED_REVIEW_COMMIT = '8a95613435f9bdf433e98ebf136a5e01a13079e5'


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


def verify_identity(size, digest, item):
    if type(item.get('bytes')) is not int or item['bytes'] < 0 or not isinstance(item.get('sha256'), str) or not re.fullmatch('[0-9a-f]{64}', item['sha256']):
        raise ValueError('Invalid evidence file identity')
    if size != item['bytes'] or digest != item['sha256']:
        raise ValueError('Evidence size or SHA256 mismatch')


def verify_file(path, item):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    verify_identity(path.stat().st_size, digest.hexdigest(), item)


def archive_inventory(path):
    """Stream archive members through tar; never extract evidence onto disk."""
    if not path.is_file():
        raise FileNotFoundError(path)
    tar = shutil.which('tar')
    if not tar:
        raise OSError('tar is required to verify packed evidence')
    files, seen = {}, set()
    with tempfile.TemporaryFile() as errors:
        process = subprocess.Popen([tar, '-cf', '-', '--format', 'pax', '@' + str(path)],
                                   stdout=subprocess.PIPE, stderr=errors,
                                   creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        try:
            with tarfile.open(fileobj=process.stdout, mode='r|') as archive:
                for member in archive:
                    name = member.name
                    parts = PurePosixPath(name).parts
                    if not parts or parts[0] != path.stem or '..' in parts or \
                            PurePosixPath(name).as_posix() != name or '\\' in name or ':' in name or name in seen:
                        raise ValueError('Invalid or duplicate archive member: ' + name)
                    seen.add(name)
                    if member.isdir():
                        continue
                    if not member.isfile() or member.issym() or member.islnk():
                        raise ValueError('Archive member is not a regular file: ' + name)
                    digest, size = hashlib.sha256(), 0
                    with archive.extractfile(member) as stream:
                        for block in iter(lambda: stream.read(1024 * 1024), b''):
                            digest.update(block)
                            size += len(block)
                    if size != member.size:
                        raise ValueError('Truncated archive member: ' + name)
                    files[name] = (size, digest.hexdigest())
            process.stdout.close()
            if process.wait(timeout=60):
                errors.seek(0)
                raise ValueError('Archive read failed: ' + errors.read().decode(errors='replace'))
        except BaseException as error:
            process.stdout.close()
            if process.poll() is None:
                process.kill()
            process.wait()
            if isinstance(error, tarfile.TarError):
                raise ValueError('Invalid evidence archive: ' + str(path)) from error
            raise
    return files


def retired_review(root, item):
    result = subprocess.run(['git', '--no-optional-locks', 'show', RETIRED_REVIEW_COMMIT + ':' + item['path']],
                            cwd=root, capture_output=True, timeout=30,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if result.returncode:
        raise ValueError('Retired review source cannot be verified: ' + item['path'])
    verify_identity(len(result.stdout), hashlib.sha256(result.stdout).hexdigest(), item)


def read_relocated_text(root, source):
    """Read a registered Markdown member without dropping its anchor checks."""
    data = json.loads((root / REGISTRY).read_text(encoding='utf-8'))
    item = next(row for row in data['files'] if root / row['source'] == source)
    target = registered_path(root, item['target'], 'docs/verification/archive/')
    member = target.relative_to(root / 'docs/verification/archive').as_posix()
    stem = PurePosixPath(member).parts[0]
    if stem not in PACKED_ARCHIVES:
        raise ValueError('Unregistered packed evidence scope')
    archive = registered_path(root, 'docs/verification/archive/' + stem + '.7z', 'docs/verification/archive/')
    tar = shutil.which('tar')
    if not tar:
        raise OSError('tar is required to read packed evidence')
    result = subprocess.run([tar, '-xOf', str(archive), member], capture_output=True, timeout=60,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    if result.returncode:
        raise ValueError('Cannot read registered archive member: ' + member)
    verify_identity(len(result.stdout), hashlib.sha256(result.stdout).hexdigest(), item)
    return result.stdout.decode('utf-8-sig')


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
    targets, expected_new, packed = {}, set(), {}

    def packed_target(target):
        relative = target.relative_to(root / 'docs/verification/archive').as_posix()
        stem = PurePosixPath(relative).parts[0]
        if stem not in PACKED_ARCHIVES:
            raise ValueError('Unregistered packed evidence scope')
        archive = registered_path(root, 'docs/verification/archive/' + stem + '.7z', 'docs/verification/archive/')
        if archive not in packed:
            packed[archive] = archive_inventory(archive)
        if relative not in packed[archive]:
            raise ValueError('Registered evidence member is missing: ' + relative)
        return archive, packed[archive][relative]
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
        if target.exists():
            verify_file(target, item)
            targets[source] = target
        else:
            archive, (size, digest) = packed_target(target)
            verify_identity(size, digest, item)
            targets[source] = archive
    new_root = root / NEW_ARCHIVE
    actual_new = set()
    if new_root.exists():
        registered_path(root, NEW_ARCHIVE.rstrip('/'), 'docs/verification/archive/')
        for path in new_root.rglob('*'):
            registered_path(root, path.relative_to(root).as_posix(), NEW_ARCHIVE)
            if path.is_file():
                actual_new.add(path)
    elif expected_new:
        archive = registered_path(root, NEW_ARCHIVE.rstrip('/') + '.7z', 'docs/verification/archive/')
        actual_new = {root / 'docs/verification/archive' / name for name in packed[archive]}
    if actual_new != expected_new:
        raise ValueError('New archive file set differs from registered inventory')
    reviews, seen_reviews = set(), set()
    for item in data['readOnlyReviewSources']:
        source = registered_path(root, item['path'], 'docs/spec/')
        if source.suffix != '.md' or source in seen_reviews:
            raise ValueError('Invalid or duplicate read-only review identity')
        seen_reviews.add(source)
        if source.exists():
            verify_file(source, item)
            reviews.add(source)
        else:
            # These sources were explicitly retired; verify their recorded Git bytes.
            retired_review(root, item)
    targets.update(change_archive_relocations(root, data))
    return targets, reviews
