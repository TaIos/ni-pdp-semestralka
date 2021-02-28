import sys
import shlex
import subprocess

from threading import Thread
from pathlib import Path


def run(command, line, **kwargs):
    command = [command, line]
    cp = subprocess.run(command,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        universal_newlines=True,
                        **kwargs)
    print(cp.stderr)
    print("======")
    print(cp.stdout)


paths = Path('../data').glob('**/*.txt')
for path in paths:
    print(path)
    run(command='./run.out', line=str(path))
    break
