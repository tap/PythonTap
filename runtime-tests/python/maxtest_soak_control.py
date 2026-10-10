# Runtime-test fixture (runtime-tests/run.py copies it into python/): the soak's tap.python (plan 9.5),
# answering a metro while maxtest_editor bumps REVISION every second.
REVISION = 0


class maxtest_soak_control:
    def int(self, n: int) -> int:
        return n
