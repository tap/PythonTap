# Test fixture: wide.py's return hints as strings that cannot be resolved (a name imported only for
# type checking), so they are read as written: split at their top level only (plan 9.1, audit m4).
from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from nowhere import Meta  # noqa: F401 — never importable: forces the as-written path


class wide_strings:
    def nested(self) -> tuple[list[Meta], dict[str, int]]:
        return [1, 2], 3

    def triple(self) -> tuple[int, int, int]:
        return 1, 2, 3

    def single(self) -> int:
        return 1
