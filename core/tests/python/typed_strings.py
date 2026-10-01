# Test fixture: the same shapes written as strings that cannot be resolved (a name imported only
# for type checking), so the annotations are read as written (plan 3.3, 8.6).
from __future__ import annotations

from typing import TYPE_CHECKING, Annotated, Optional

if TYPE_CHECKING:
    from nowhere import Meta  # noqa: F401 — never importable: forces the as-written path


class typed_strings:
    noted: Annotated[float, Meta] = 0.5
    either: float | int = 1.0
    maybe: Optional[Annotated[int, Meta]] = 2

    def process(self, x: float) -> tuple[float, float] | None:
        return x, x
