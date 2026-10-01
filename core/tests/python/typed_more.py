# Test fixture: hint shapes a reader expects to map as their payload does (plan 8.6, audit A8).
from typing import Annotated, Final, Optional


class typed_more:
    noted: Annotated[float, "metadata"] = 0.5          # float
    fixed: Final[int] = 3                              # int
    either: float | int = 1.0                          # several kinds: the atom as it comes
    text_or_number: str | float = "a"                  # likewise
    maybe_noted: Optional[Annotated[float, "m"]] = 2.0  # float

    def process(self, x: float) -> Optional[tuple[float, float]]:  # two outputs
        return x, -x
