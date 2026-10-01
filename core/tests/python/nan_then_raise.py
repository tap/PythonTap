# Test fixture: NaN output until an input of 0.75 or more, which raises (plan 8.2, audit A2):
# the samples written before the exception must still be sanitized.


class nan_then_raise:
    def process(self, x: float) -> float:
        if x >= 0.75:
            raise RuntimeError("nan_then_raise: raised on purpose")
        return float("nan")
