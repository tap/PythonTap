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

    # Named like messages Max sends with C arguments (plan 8.2): the object must not expose them,
    # or Max's call would crash it when DSP toggles, a cord connects, or a collective is built.
    def dspstate(self, on: int) -> None:
        self._fault = "dspstate was called"

    def patchlineupdate(self) -> None:
        self._fault = "patchlineupdate was called"

    def inputchanged(self) -> None:
        self._fault = "inputchanged was called"

    def fileusage(self) -> None:
        self._fault = "fileusage was called"

    def process(self, x: float) -> float:
        if self._fault == "raise":
            raise RuntimeError("maxtest: process() raised on purpose")
        if self._fault == "nan":
            return float("nan")
        return x
