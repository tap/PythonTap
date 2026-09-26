import numpy as np
from attrs import define, field


@define
class numpy_gain:
    """A gain, processed a whole signal vector at a time with numpy.

    Because process() is annotated np.ndarray, tap.python~ calls it once per signal
    vector instead of once per sample — far cheaper, and the way to write anything
    that has to run in real time. x is reused from one call to the next: copy it
    if you want to keep it.
    """

    gain: float = field(default = 1.0)

    def process(self, x: np.ndarray) -> np.ndarray:
        return x * self.gain
