import sys
from pathlib import Path
# Installed packages carry these helpers beside the launcher.
if (Path(__file__).parent / "scripts_common.py").exists():
    sys.path.insert(0, str(Path(__file__).parent))
else:
    sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import cli
from manual_input import run
if __name__ == "__main__":
    cli(lambda: run("package", Path(__file__).parent))
