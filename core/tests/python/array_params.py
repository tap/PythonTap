# Test fixture: a message parameter hinted np.ndarray takes the atoms left after the others, as a
# float64 array (plan 9.1).
import numpy as np


class array_params:
    got: str = ""

    def take(self, values: np.ndarray) -> None:
        self.got = repr((type(values).__name__, values.dtype.name, values.tolist()))

    def scaled(self, gain: float, values: np.ndarray) -> np.ndarray:
        return values * gain
