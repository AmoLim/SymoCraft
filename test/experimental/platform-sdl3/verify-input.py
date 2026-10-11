import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import cli
from platform_verification import run
if __name__ == "__main__":
    cli(lambda: run("input", Path(__file__).parent))
