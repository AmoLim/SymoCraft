"""Compile production public API misuse cases; require the intended diagnostic."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--configuration", choices=("Debug", "Release"), required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    output_directory = Path(tempfile.mkdtemp(prefix="run-", dir=args.output))
    markers = {
        1: "SYMOCRAFT_ECS_BARE", 2: "SYMOCRAFT_ECS_BARE", 3: "SYMOCRAFT_ECS_READ",
        4: "C3490", 5: "C3490", 6: "SYMOCRAFT_ECS_BARE", 7: "SYMOCRAFT_ECS_BARE",
        8: "SYMOCRAFT_ECS_BARE", 9: "SYMOCRAFT_ECS_BARE", 10: "SYMOCRAFT_ECS_QUERY_TYPES",
        11: "SYMOCRAFT_ECS_QUERY_TYPES", 12: "SYMOCRAFT_ECS_BARE", 13: "SYMOCRAFT_ECS_VISITOR",
        14: "SYMOCRAFT_ECS_VISITOR", 15: "C2794", 16: "C2794", 17: "C2794",
        18: "SYMOCRAFT_ECS_DEFAULT", 19: "SYMOCRAFT_ECS_LIFETIME", 20: "C2280", 21: "C2280",
        22: "C2248", 23: "C2440", 24: "C2676", 25: "SYMOCRAFT_ECS_QUERY_TYPES",
        26: "SYMOCRAFT_ECS_VISITOR", 27: "C2794", 28: "SYMOCRAFT_ECS_READ", 29: "SYMOCRAFT_ECS_CTOR",
    }
    source = args.root / "test/unit/ecs/registry_compile_negative.cpp"
    for number, marker in markers.items():
        result = subprocess.run([args.compiler, "/nologo", "/c", "/EHsc", "/std:c++20", "/Zc:__cplusplus",
            "/DENTT_NO_ETO", "/D_ITERATOR_DEBUG_LEVEL=0", f"/DNEGATIVE_CASE={number}",
            "/MDd" if args.configuration == "Debug" else "/MD",
            f"/I{args.root / 'game/modules/ecs/include'}", f"/I{args.root / 'vendor/entt/src'}",
            f"/Fo{output_directory / f'negative-{number}.obj'}", str(source)], capture_output=True)
        output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
        (output_directory / f"negative-{number}.log").write_text(output, encoding="utf-8")
        if result.returncode == 0 or marker not in output:
            raise RuntimeError(f"Missing intended rejection #{number}: {marker}")
        if marker.startswith("SYMOCRAFT_") and "error C2338" not in output:
            raise RuntimeError(f"Missing project static assertion #{number}")
        if marker.startswith("C") and f"registry_compile_negative.cpp(" not in output:
            raise RuntimeError(f"Missing source location #{number}")
    print(f"PASS production API expected compile rejections: {len(markers)}")


if __name__ == "__main__":
    main()
