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
    return list(map(int, cp.stdout.strip().split()))


paths = Path('../data').glob('**/*.txt')
out = Path('./out')

for path in paths:
    r = run(command='./run.out', line=path)
    r.append(path.name)
    print(r)
    break
