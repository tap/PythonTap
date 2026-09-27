# Runtime-test fixture (runtime-tests/run.py copies it into python/): a per-sample gain.


class maxtest_gain:
    gain: float = 1.0

    def process(self, x: float) -> float:
        return x * self.gain
