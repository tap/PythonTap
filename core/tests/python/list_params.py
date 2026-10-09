# Test fixture: message parameters hinted list[...] — the last one takes the atoms left after the
# others (plan 9.1).


class list_params:
    got: str = ""

    def take(self, values: list[float]) -> None:
        self.got = repr(values)

    def ints(self, values: list[int]) -> None:
        self.got = repr(values)

    def words(self, values: list[str]) -> None:
        self.got = repr(values)

    def anything_list(self, values: list) -> None:
        self.got = repr(values)

    def written(self, values: "list[int]") -> None:
        self.got = repr(values)

    def after(self, first: int, values: list[float]) -> None:
        self.got = repr((first, values))

    def optional(self, values: list[float] | None = None) -> None:
        self.got = repr(values)

    def not_last(self, values: list[float], *more) -> None:
        self.got = repr((values, more))
