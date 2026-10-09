# Test fixture: message parameters hinted list[...] or np.ndarray (plan 9.1).


class list_params:
    got: str = ""

    def take(self, values: list[float]) -> None:
        self.got = repr(values)
