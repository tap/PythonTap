# Runtime-test fixture (runtime-tests/run.py copies it into python/): misbehaves on request.
import sys


class maxtest_faults:
    def __init__(self):
        self._fault = ""

    def exit(self, code: int) -> None:
        sys.exit(code)

    def nan(self) -> None:
        self._fault = "nan"

    def fail(self) -> None:
        self._fault = "raise"

    def heal(self) -> None:
        self._fault = ""

    def process(self, x: float) -> float:
        if self._fault == "raise":
            raise RuntimeError("maxtest: process() raised on purpose")
        if self._fault == "nan":
            return float("nan")
        return x
