# Test fixture: annotations that cannot be resolved at runtime (plan 3.3). numpy is imported only
# for type checkers, so typing.get_type_hints raises NameError for 'np'.
from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    import numpy as np


class bad_hints:
    level: float = 1.0
    buffer: np.ndarray = None

    def set_level(self, v: float) -> None:
        self.level = v

    def process(self, x: float) -> float:
        return x * self.level
