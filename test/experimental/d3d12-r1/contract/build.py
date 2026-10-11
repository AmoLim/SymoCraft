import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "scripts"))
from verification_common import cli
from experimental_build import run

if __name__ == "__main__":
    cli(lambda: run("contract", Path(__file__).parent))
