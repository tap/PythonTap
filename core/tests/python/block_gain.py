# Test fixture: process() hinted np.ndarray is called once per vector (plan 2.2).
import numpy as np


class block_gain:
    level: float = 0.5
    calls: int = 0

    def process(self, x: np.ndarray) -> np.ndarray:
        self.calls += 1
        return x * self.level
