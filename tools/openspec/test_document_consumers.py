"""Exercise the actual benchmark document install rules without native targets."""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
DOCUMENTS = {
    "BENCHMARK-README.zh-CN.md": "tools/benchmark/README.md",
    "THIRD-PARTY-INVENTORY.md": "docs/third-party.md",
}
INSTALL_PROJECT = r'''cmake_minimum_required(VERSION 3.22)
project(BenchmarkDocumentConsumer LANGUAGES NONE)
if(NOT DEFINED SYMOCRAFT_REPOSITORY_ROOT)
    message(FATAL_ERROR "A document source root is required.")
endif()
# Evaluate the two production declarations verbatim, not duplicated fixture rules.
file(STRINGS "${SYMOCRAFT_REPOSITORY_ROOT}/tools/benchmark/CMakeLists.txt"
    document_rules REGEX "^install\\(FILES ")
list(LENGTH document_rules rule_count)
if(NOT rule_count EQUAL 2)
    message(FATAL_ERROR "Review changed benchmark document install declarations.")
endif()
foreach(rule IN LISTS document_rules)
    cmake_language(EVAL CODE "${rule}")
endforeach()
'''


class DocumentConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cmake = shutil.which("cmake")
        if cls.cmake is None:
            raise RuntimeError("CMake is required to exercise document installation.")

    def setUp(self):
        output = ROOT / "out"
        output.mkdir(exist_ok=True)
        self.temporary = tempfile.TemporaryDirectory(prefix="document-consumers-", dir=output)
        self.root = Path(self.temporary.name).resolve()
        if not self.root.is_relative_to(output.resolve()):
            raise RuntimeError("The isolated fixture must stay inside workspace out.")
        self.addCleanup(self.temporary.cleanup)
        self.project = self.root / "install project"
        self.project.mkdir()
        (self.project / "CMakeLists.txt").write_text(INSTALL_PROJECT, encoding="utf-8")

    def configure(self, repository):
        self.build = self.root / "build"
        configured = subprocess.run(
            [self.cmake, "-S", str(self.project), "-B", str(self.build),
             "-G", "Ninja", f"-DSYMOCRAFT_REPOSITORY_ROOT={repository.as_posix()}"],
            capture_output=True, text=True, encoding="utf-8", timeout=60,
        )
        self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)

    def install(self):
        self.destination = self.root / "installed \u5b89\u88c5 package"
        return subprocess.run(
            [self.cmake, "--install", str(self.build), "--prefix", str(self.destination)],
            capture_output=True, text=True, encoding="utf-8", timeout=60,
        )

    def fixture_repository(self):
        repository = self.root / "source \u6d4b\u8bd5"
        cmake_source = repository / "tools/benchmark/CMakeLists.txt"
        cmake_source.parent.mkdir(parents=True)
        cmake_source.write_bytes((ROOT / "tools/benchmark/CMakeLists.txt").read_bytes())
        return repository

    def test_actual_rules_install_canonical_documents_verbatim(self):
        self.configure(ROOT)
        installed = self.install()
        self.assertEqual(installed.returncode, 0, installed.stdout + installed.stderr)
        self.assertEqual({path.name for path in self.destination.iterdir()}, set(DOCUMENTS))
        for name, relative in DOCUMENTS.items():
            self.assertEqual((self.destination / name).read_bytes(), (ROOT / relative).read_bytes())

    def test_actual_rules_need_no_legacy_inputs(self):
        repository = self.fixture_repository()
        for relative in DOCUMENTS.values():
            source = repository / relative
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_bytes((ROOT / relative).read_bytes())
        self.assertFalse((repository / "docs/project/benchmark").exists())
        self.assertFalse((repository / "docs/legacy").exists())
        self.configure(repository)
        installed = self.install()
        self.assertEqual(installed.returncode, 0, installed.stdout + installed.stderr)
        for name, relative in DOCUMENTS.items():
            self.assertEqual((self.destination / name).read_bytes(), (repository / relative).read_bytes())

    def test_actual_rules_reject_missing_canonical_source(self):
        repository = self.fixture_repository()
        (repository / "tools/benchmark/README.md").write_bytes(b"operating fixture\r\n")
        self.configure(repository)
        installed = self.install()
        self.assertNotEqual(installed.returncode, 0)
        self.assertFalse((self.destination / "THIRD-PARTY-INVENTORY.md").exists())
        self.assertIn("third-party.md", installed.stdout + installed.stderr)


if __name__ == "__main__":
    unittest.main()
