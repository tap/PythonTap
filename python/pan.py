import numpy as np
from attrs import define, field

@define
class pan:
    position: float = field(default = 0.5)

    def process(self, x: float) -> tuple:
        y_left = x * self.position
        y_right = x * (1.0 - self.position)
        return (y_left, y_right)
