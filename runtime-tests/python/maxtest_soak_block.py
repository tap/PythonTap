# Runtime-test fixture (runtime-tests/run.py copies it into python/): the soak test's block-path
# class (plan 6.2). maxtest_editor bumps REVISION every second.
import numpy as np

REVISION = 0


class maxtest_soak_block:
    gain: float = 1.0

    def process(self, x: np.ndarray) -> np.ndarray:
        return x * self.gain
