# Runtime-test fixture (runtime-tests/run.py copies it into python/): one file for a tap.python~ and a
# tap.python at once. maxtest_editor bumps REVISION (one execution of the file for both) and scales
# SCALE. Its methods mode and dumpout are each reserved by one kind of object and not the other — mode
# by tap.python~, dumpout by tap.python — so each kind says once what it owes, whichever ran the file.
REVISION = 0
SCALE = 1.0


class maxtest_shared:
    def process(self, x: float) -> float:
        return x * SCALE

    def value(self) -> float:
        return SCALE

    def mode(self) -> None:
        pass

    def dumpout(self) -> None:
        pass
