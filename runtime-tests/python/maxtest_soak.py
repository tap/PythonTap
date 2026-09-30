# Runtime-test fixture (runtime-tests/run.py copies it into python/): the soak test's per-sample
# class (plan 6.2). maxtest_editor bumps REVISION every second; each save is one execution of this
# module, shared by every instance, and counted where a fresh module cannot reset it.
import gc
import sys

REVISION = 0
sys._maxtest_soak_executions = getattr(sys, "_maxtest_soak_executions", 0) + 1


class maxtest_soak:
    gain: float = 1.0
    executions: int = 0
    objects: int = 0

    def census(self) -> None:
        """How many times this file has run, and how many objects Python is tracking."""
        self.executions = sys._maxtest_soak_executions
        self.objects = len(gc.get_objects())

    def process(self, x: float) -> float:
        return x * self.gain
