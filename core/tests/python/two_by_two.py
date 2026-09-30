# Test fixture: two inputs and two outputs, per sample (plan 2.4).


class two_by_two:
    def process(self, a: float, b: float) -> tuple[float, float]:
        return a + b, a - b
