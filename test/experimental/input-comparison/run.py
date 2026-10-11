import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))
from verification_common import cli
from manual_input import run
if __name__ == "__main__":
    cli(lambda: run("comparison", Path(__file__).parent))
