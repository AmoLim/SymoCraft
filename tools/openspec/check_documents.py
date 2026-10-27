"""Audit local documentation; optionally regenerate its non-authoritative tag index."""

import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import unicodedata
from urllib.parse import unquote, urlsplit
import evidence_paths

ROOT = Path(__file__).resolve().parents[2]
PRIVATE = {'.obsidian', 'development-journal'}
RETIREMENT = 'docs/verification/archive/project-history-20261011/retirement-manifest.json'


def documents():
    for directory, dirs, names in os.walk(ROOT / 'docs'):
        dirs[:] = [name for name in dirs if name not in PRIVATE | {'evidence', 'archive', 'templates'}]
        for name in names:
            path = Path(directory) / name
            if name.endswith('.md') and name not in {'source-migration.tsv.md', 'openspec-migration-context.tmp.md'}:
                yield path
    for path in (ROOT / 'AGENTS.md', ROOT / 'README.md', ROOT / 'tools/benchmark/README.md', ROOT / 'scripts/README.md'):
        yield path


def mask_fences(body):
    body = re.sub(r'(?ms)^\s*(`{3,}|~{3,})[^\n]*\n.*?^\s*\1\s*$', lambda match: ' ' * len(match[0]), body)
    return body


def mask_code(body):
    return re.sub(r'`[^`\n]+`', lambda match: ' ' * len(match[0]), mask_fences(body))


def link_matches(body):
    body = mask_code(body)
    inline = list(re.finditer(r'!?\[[^\]\n]*\]\(\s*(<[^>]+>|[^)\n]+)\)', body))
    reference = list(re.finditer(r'^\s*\[[^\]\n]+\]:\s*(<[^>]+>|\S+)', body, re.M))
    return sorted(inline + reference, key=lambda match: match.start())


def slugs(body):
    body = mask_fences(body)
    result = set(re.findall(r'<a\s+(?:id|name)=["\x27]([^"\x27]+)', body))
    seen = collections.Counter()
    for title in re.findall(r'^#{1,6}\s+(.+?)\s*#*\s*$', body, re.M):
        title = re.sub(r'\[([^\]]+)\]\([^)]*\)', r'\1', title)
        slug = ''.join(char for char in title.lower() if char in '- _' or unicodedata.category(char)[0] in 'LN').replace(' ', '-')
        count = seen[slug]
        seen[slug] += 1
        result.add(slug + (f'-{count}' if count else ''))
    return result


def tags(body):
    front = body.split('---', 2)[1] if body.startswith('---') and body.count('---') >= 2 else ''
    match = re.search(r'(?m)^tags:\s*\n((?:[ \t]+- [^\n]+\n?)+)', front)
    return re.findall(r'(?m)^\s*-\s*(\S+)\s*$', match[1]) if match else []


def retired_review_source(relative, body):
    # Retired notes remain fully audited; this only removes their current index entry.
    return relative.startswith('spec/') or bool(re.search(
        r'^> 2026-10-(?:10|11) 文档浓缩后已退役复核源：', mask_fences(body), re.M))


def historical_relocations():
    """Resolve only byte-registered, read-only review links to relocated originals."""
    registry = ROOT / RETIREMENT
    if not registry.exists():
        return {}, set(), []

    def registered_path(relative, prefix):
        if not isinstance(relative, str) or not relative.startswith(prefix) or \
                Path(relative).as_posix() != relative or '..' in Path(relative).parts:
            raise ValueError('Invalid registered path')
        path = ROOT / relative
        if path.resolve() != path.absolute() or not path.resolve().is_relative_to(ROOT):
            raise ValueError('Registered path escaped its original location')
        for parent in (path, *path.parents):
            if parent == ROOT.parent:
                break
            if parent.exists() and getattr(parent.lstat(), 'st_file_attributes', 0) & \
                    getattr(stat, 'FILE_ATTRIBUTE_REPARSE_POINT', 0x400):
                raise ValueError('Registered path uses a reparse point')
        return path

    def verify_file(path, item):
        if not path.is_file() or type(item['bytes']) is not int or item['bytes'] < 0 or \
                not isinstance(item['sha256'], str) or not re.fullmatch('[0-9a-f]{64}', item['sha256']):
            raise ValueError('Invalid historical file identity')
        raw = path.read_bytes()
        if len(raw) != item['bytes'] or hashlib.sha256(raw).hexdigest() != item['sha256']:
            raise ValueError('Historical file size or SHA256 mismatch: ' + path.relative_to(ROOT).as_posix())

    try:
        registered_path(RETIREMENT, 'docs/verification/archive/project-history-20261011/')
        record = json.loads(registry.read_text(encoding='utf-8'))
        if record['schemaVersion'] != 1 or not isinstance(record['documents'], list) or \
                not isinstance(record['readOnlyReviewSources'], list):
            raise ValueError('Invalid historical registry schema')
        targets, reviews = {}, set()
        for item in record['documents']:
            source = registered_path(item['source'], 'docs/project/')
            target = registered_path(item['destination'], 'docs/verification/archive/project-history-20261011/')
            if source.suffix != '.md' or target != registry.parent / source.relative_to(ROOT / 'docs/project') or source in targets:
                raise ValueError('Duplicate or inconsistent historical relocation')
            verify_file(target, item)
            targets[source] = target
        for item in record['readOnlyReviewSources']:
            source = registered_path(item['path'], 'docs/spec/')
            if source.suffix != '.md' or source in reviews:
                raise ValueError('Duplicate or invalid read-only review source')
            verify_file(source, item)
            reviews.add(source)
        return targets, reviews, []
    except (OSError, ValueError, KeyError, TypeError) as error:
        return {}, set(), [{'file': RETIREMENT, 'problem': 'invalid historical relocation registry',
                             'detail': str(error)}]


def audit():
    texts = {path: path.read_text(encoding='utf-8-sig') for path in documents()}
    by_name = collections.defaultdict(list)
    for path in texts:
        by_name[path.name].append(path)
    has_reviews = any(path.is_relative_to(ROOT / 'docs/spec') for path in texts)
    relocations, historical_sources, errors = historical_relocations() if has_reviews else ({}, set(), [])
    try:
        evidence, reviews = evidence_paths.load_relocations(ROOT)
        relocations.update(evidence)
        historical_sources.update(reviews)
    except (OSError, ValueError, KeyError, TypeError) as error:
        errors.append({'file': evidence_paths.REGISTRY, 'problem': 'invalid evidence relocation registry', 'detail': str(error)})
    warnings, links, wikis, historical_links = [], 0, 0, 0
    for path, body in texts.items():
        for match in link_matches(body):
            raw = match[1]
            url = raw[1:-1] if raw.startswith('<') else re.sub(r'\s+"[^"\n]*"\s*$', '', raw)
            parsed = urlsplit(url)
            if parsed.scheme or parsed.netloc:
                continue
            links += 1
            target = (path.parent / unquote(parsed.path)).resolve() if parsed.path else path
            original_target = target
            relocated = False
            if not target.exists() and path in historical_sources and target in relocations:
                target = relocations[target]
                relocated = True
                historical_links += 1
            problem = None
            if not target.is_relative_to(ROOT):
                problem = 'outside repository'
            elif not target.exists():
                problem = 'missing target'
            elif relocated and parsed.fragment and original_target.suffix.lower() == '.md' and target.suffix.lower() == '.7z':
                if unquote(parsed.fragment) not in slugs(evidence_paths.read_relocated_text(ROOT, original_target)):
                    problem = 'missing anchor'
            elif parsed.fragment and target.is_file() and target.suffix.lower() == '.md' and unquote(parsed.fragment) not in slugs(
                    texts[target] if target in texts else target.read_text(encoding='utf-8-sig')):
                problem = 'missing anchor'
            if problem:
                item = {'file': path.relative_to(ROOT).as_posix(), 'url': url, 'problem': problem}
                errors.append(item)
        wiki_body = body.split('\n## 附录归档', 1)[0] if path.name == 'M3-T4-应用平台与模拟模块接口.md' else body
        for value in re.findall(r'\[\[([^\]\n]+)\]\]', mask_code(wiki_body)):
            wikis += 1
            destination, _, anchor = value.replace('\\|', '|').split('|', 1)[0].partition('#')
            if not destination:
                candidates = [path]
            elif '/' not in destination and '\\' not in destination:
                candidates = by_name.get(Path(destination).name.removesuffix('.md') + '.md', [])
            else:
                suffix = '' if destination.endswith('.md') else '.md'
                candidates = [candidate.resolve() for candidate in (path.parent / (destination + suffix), ROOT / 'docs' / (destination + suffix)) if candidate.resolve() in texts]
                candidates = list(set(candidates))
            if len(candidates) != 1 or anchor and anchor not in re.findall(r'^#{1,6}\s+(.+?)\s*#*\s*$', texts.get(candidates[0], ''), re.M):
                errors.append({'file': path.relative_to(ROOT).as_posix(), 'wiki': value, 'problem': 'unresolved wiki target/heading'})
        if path.is_relative_to(ROOT / 'docs'):
            areas = [tag for tag in tags(body) if tag.startswith('area/')]
            if len(areas) != 1:
                errors.append({'file': path.relative_to(ROOT).as_posix(), 'problem': 'expected one area tag'})
            elif path.is_relative_to(ROOT / 'docs/openspec') and areas != ['area/spec']:
                errors.append({'file': path.relative_to(ROOT).as_posix(), 'problem': 'OpenSpec requires area/spec'})
    return {'documents': len(texts), 'links': links, 'wikiLinks': wikis,
            'historicalLinks': historical_links, 'errors': errors, 'warnings': warnings}


def write_index():
    destination = ROOT / 'docs/tag-inventory.json'
    source = destination if destination.is_file() else ROOT / 'docs/spec/md-organize/tag-inventory.json'
    previous = json.loads(source.read_text(encoding='utf-8'))
    private = previous.get('preservedPrivateEntries', [item for item in previous.get('managed', [])
                                                     if item['path'].startswith('development-journal/')])
    records, retired = [], []
    for path in sorted(documents()):
        if not path.is_relative_to(ROOT / 'docs'):
            continue
        relative = path.relative_to(ROOT / 'docs').as_posix()
        body = path.read_text(encoding='utf-8-sig')
        if retired_review_source(relative, body):
            retired.append(relative)
            continue
        values = tags(body)
        areas = [value for value in values if value.startswith('area/')]
        if len(areas) != 1:
            raise RuntimeError(f'Cannot index ambiguous ownership: {relative}')
        if relative.startswith('openspec/') and areas != ['area/spec']:
            raise RuntimeError(f'OpenSpec requires area/spec: {relative}')
        records.append({'path': relative, 'area': areas[0], 'tags': values})
    private = [item for item in private if (ROOT / 'docs' / item['path']).is_file()]
    areas = collections.Counter(item['area'].removeprefix('area/') for item in records)
    result = {'schemaVersion': 1, 'vaultRoot': 'docs', 'spec': 'DOCUMENTATION_MAP.md',
              'uiVerification': 'not-run',
              'counts': {'managed': len(records), 'areas': dict(sorted(areas.items())),
                         'retiredReviewSources': len(retired), 'preservedPrivateRegistrations': len(private)},
              'managed': records, 'retiredReviewSources': retired,
              'preservedPrivateEntries': private,
              'exclusionRules': ['**/evidence/**', 'verification/archive/**', 'openspec/schemas/*/templates/**',
                                 'development-journal/**', '.obsidian/**', '*context.tmp.md', 'source-migration.tsv.md']}
    destination.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8', newline='\n')
    return result['counts']


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--write-index', action='store_true', help='Regenerate docs/tag-inventory.json, then audit the documentation')
    options = parser.parse_args()
    counts = write_index() if options.write_index else None
    result = audit()
    if counts is not None:
        result['index'] = counts
    print(json.dumps(result, ensure_ascii=False, indent=2))
    raise SystemExit(bool(result['errors']))
