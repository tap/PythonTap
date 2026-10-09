# Test fixture: return hints that name several outlets (plan 9.1); see wide_strings.py for the same
# written as strings that cannot be resolved.


class wide:
    def nested(self) -> tuple[list[int], dict[str, int]]:
        return [1, 2], 3

    def triple(self) -> tuple[int, int, int]:
        return 1, 2, 3

    def single(self) -> int:
        return 1
