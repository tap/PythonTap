# Test fixture: declares two outputs, returns three values (plan 2.4).


class wrong_count:
    def process(self, x: float) -> tuple[float, float]:
        return x, x, x
