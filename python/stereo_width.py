import numpy as np
from attrs import define, field


@define
class stereo_width:
    """Stereo width: two signal inputs and two outputs, left and right.

    process()'s parameters are the object's signal inlets — [tap.python~ stereo_width] has two —
    and its return hint, tuple[np.ndarray, np.ndarray], gives it two outlets. The signal is split
    into what the channels share (mid) and what differs (side); width scales the side: 0 folds
    the pair to mono, 1 leaves it as it was, more than 1 widens it.
    """

    width: float = field(default = 1.0)

    @width.validator
    def _check_width(self, attribute, value):
        if value < 0.0:
            raise ValueError("width must be 0 or more")

    def process(self, left: np.ndarray, right: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        mid = (left + right) * 0.5
        side = (left - right) * (0.5 * self.width)
        return mid + side, mid - side
