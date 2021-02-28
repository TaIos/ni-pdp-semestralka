import sys
import shlex
import subprocess
import time

from threading import Thread
from pathlib import Path


class Task:

    def __init__(self, command, line, filename):
        self.command = command
        self.line = line
        self.filename = filename
        self.result = None

    def run(self, **kwargs):
        command = [self.command, self.line]
        cp = subprocess.run(command,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True,
                            **kwargs)
        self.result = list(map(int, cp.stdout.strip().split()))
        print(self)

    def __str__(self):
        return self.filename + ' ' + ' '.join(str(x) for x in self.result)


tasks = []
paths = Path('../data').glob('**/*.txt')
for path in paths:
    tasks.append(Task(command='./run.out', line=path, filename=path.stem))

threads = []
for task in tasks:
    t = Thread(target=task.run)
    threads.append(t)
    t.start()

for t in threads:
    t.join()

out = Path(f'./out/{time.strftime("%Y%m%d-%H%M%S")}.txt')
with out.open('w', encoding="utf-8") as f:
    for task in tasks:
        f.write(str(task))
