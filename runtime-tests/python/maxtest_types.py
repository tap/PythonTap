# Runtime-test fixture (runtime-tests/run.py copies it into python/): one attribute of each hint
# kind, and messages of several signatures.
from typing import Optional


class maxtest_types:
    gain: float = 1.0
    count: int = 3
    on: bool = True
    label: str = "start"
    limit: Optional[float] = None

    def __init__(self):
        self._offset = 0.0

    def add(self, amount: float) -> None:
        self._offset += amount

    def reset(self) -> None:
        self._offset = 0.0

    def pair(self, a: int, b: float) -> None:
        self.count = a
        self.gain = b

    def rename(self, name: str) -> None:
        self.label = name

    def bang(self) -> None:
        self.count += 1

    def process(self, x: float) -> float:
        y = x * self.gain + self._offset if self.on else 0.0
        return y if self.limit is None else min(y, self.limit)
