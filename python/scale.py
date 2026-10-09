import numpy as np
from attrs import define, field


@define
class scale:
    """A list in, scaled and offset with numpy, a list out, for tap.python: with @factor 2 and
    @offset 1, the list 1 2 3 outputs 3 5 7.

    A last parameter hinted np.ndarray takes every atom of the message as one float64 array, and an
    array returned is output as a list — so a list message runs through numpy and back.
    """

    factor: float = field(default = 1.0)
    offset: float = field(default = 0.0)

    # `list` comes last on purpose: once `def list` has run, the name `list` in the class body means
    # this method, so an annotation written after it would no longer mean the built-in type. (Inside
    # method bodies the built-ins are unaffected.)

    def list(self, values: np.ndarray) -> np.ndarray:
        return values * self.factor + self.offset
