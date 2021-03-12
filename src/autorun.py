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

        it = iter(cp.stdout.split('\n'))
        for line in it:
            if "Cena" in line:
                r = next(it)
                break

        self.result = list(map(int, r.strip().split()))
        print(self)
        self._check_result()

    def _check_result(self):
        n = int(self.filename[3:])
        r = self.result[0]

        if n == 1:
            self._my_assert(r, 5)
        if n == 2:
            self._my_assert(r, 14)
        if n == 3:
            self._my_assert(r, 12)
        if n == 4:
            self._my_assert(r, 14)
        if n == 5:
            self._my_assert(r, 12)
        if n == 6:
            self._my_assert(r, 16)
        if n == 7:
            self._my_assert(r, 16)
        if n == 8:
            self._my_assert(r, 5)
        if n == 9:
            self._my_assert(r, 26)
        if n == 10:
            self._my_assert(r, 20)
        if n == 11:
            self._my_assert(r, 18)
        if n == 12:
            self._my_assert(r, 22)

    def _my_assert(self, got, ref):
        if got != ref:
            print(f' {ref}')

    def __str__(self):
        return self.filename + ' ' + ' '.join(str(x) for x in self.result)


pcmd = Path('../cmake-build-openmp/semestralka')

tasks = []
paths = Path('../data').glob('**/*.txt')
for path in paths:
    tasks.append(Task(command=pcmd, line=path, filename=path.stem))

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
