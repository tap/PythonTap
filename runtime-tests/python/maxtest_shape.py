# Runtime-test fixture (runtime-tests/run.py copies it into python/): two inputs and two outputs,
# each passed through by position (plan 2.4). maxtest_editor's reshape() rewrites it with another
# number of each.


class maxtest_shape:
    def process(self, x0: float, x1: float) -> tuple[float, float]:
        return (x0, x1)
