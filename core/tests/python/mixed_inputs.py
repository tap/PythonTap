# Test fixture: one input per vector and one per sample, which cannot be bound (plan 2.4).
import numpy as np


class mixed_inputs:
    def process(self, a: np.ndarray, b: float) -> np.ndarray:
        return a
