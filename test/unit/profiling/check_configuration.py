"""Exercise the real adapter without vendor, and production include restrictions."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--cmake', required=True)
    parser.add_argument('--ninja', required=True)
    args = parser.parse_args()

    def run(command, *, expected=0, contains=None):
        result = subprocess.run(command, capture_output=True, text=True, errors='replace', timeout=60)
        output = result.stdout + result.stderr
        if (expected == 0 and result.returncode != 0) or (expected != 0 and result.returncode == 0):
            raise RuntimeError(output)
        if contains and contains not in output:
            raise RuntimeError('Missing expected diagnostic: ' + contains + '\n' + output)

    with tempfile.TemporaryDirectory(prefix='symocraft-profiling-') as temp:
        root = Path(temp)
        shutil.copytree(args.repo / 'game/modules/profiling', root / 'game/modules/profiling')
        tests = args.repo / 'test/unit/profiling'
        (root / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.22)\nproject(AdapterFixture LANGUAGES CXX)\n'
            'set(SYMOCRAFT_REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")\n'
            'add_subdirectory(game/modules/profiling)\n'
            f'add_executable(probe "{tests.as_posix()}/profiling_tests.cpp" "{tests.as_posix()}/profiling_other.cpp")\n'
            'target_link_libraries(probe PRIVATE symocraft_profiling)\n', encoding='utf-8')
        configure = [args.cmake, '-S', str(root), '-G', 'Ninja', f'-DCMAKE_MAKE_PROGRAM={args.ninja}']
        run([*configure, '-B', str(root / 'off'), '-DSYMOCRAFT_ENABLE_TRACY=OFF'])
        run([args.cmake, '--build', str(root / 'off')])
        run([str(root / 'off/probe.exe')])
        run([*configure, '-B', str(root / 'on'), '-DSYMOCRAFT_ENABLE_TRACY=ON'],
            expected=1, contains='SYMOCRAFT_ENABLE_TRACY requires fixed local Tracy')

        cases = [
            ('foundation', 'game/modules/foundation/src/invalid.cpp', 'symocraft/profiling/profiling.h', False),
            ('physics', 'game/modules/simulation/physics/src/invalid.cpp', 'symocraft/profiling/profiling.h', False),
            ('world', 'game/modules/world/include/public.h', 'symocraft/profiling/profiling.h', False),
            ('world', 'game/modules/world/src/direct.cpp', 'tracy/Tracy.hpp', False),
            ('world', 'game/modules/world/src/valid.cpp', 'symocraft/profiling/profiling.h', True),
        ]
        for module, rel, header, allowed in cases:
            source = root / rel
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_text(f'#include <{header}>\n', encoding='utf-8')
            script = root / 'check.cmake'
            script.write_text(
                f'include("{args.repo.as_posix()}/cmake/ModuleBoundaries.cmake")\n'
                f'symocraft_check_source_file("{root.as_posix()}" "{module}" "{source.as_posix()}")\n', encoding='utf-8')
            run([args.cmake, '-P', str(script)], expected=0 if allowed else 1,
                contains=None if allowed else 'Boundary violation')
    print('Vendor-free OFF build/run, missing-vendor ON rejection, and source include boundaries passed.')


if __name__ == '__main__':
    main()
