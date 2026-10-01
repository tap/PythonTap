# Test fixture: numpy scalar types as field hints map to the kinds they hold (plan 8.6).
import numpy as np


class typed_numpy:
    level: np.float64 = np.float64(0.5)
    count: np.int64 = np.int64(2)
    flag: np.bool_ = np.bool_(True)

    def process(self, x: float) -> float:
        return x * float(self.level)
