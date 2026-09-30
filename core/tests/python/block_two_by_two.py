# Test fixture: two inputs and two outputs, per vector (plan 2.4).
import numpy as np


class block_two_by_two:
    def process(self, a: np.ndarray, b: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        return a + b, a - b
