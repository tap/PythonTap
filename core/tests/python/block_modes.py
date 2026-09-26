# Test fixture: the shapes a block process() result can take (plan 2.2).
import numpy as np


class block_modes:
    mode: int = 0
    last_input_id: int = 0

    def process(self, x: np.ndarray) -> np.ndarray:
        self.last_input_id = id(x)
        if self.mode == 0:
            return x * 2.0
        if self.mode == 1:
            return (x * 2.0).astype(np.float32)
        if self.mode == 2:
            x *= 2.0
            return x
        if self.mode == 3:
            return x[:-1] * 2.0
        if self.mode == 4:
            return None
        if self.mode == 5:
            return [v * 2.0 for v in x]
        return x * np.inf
