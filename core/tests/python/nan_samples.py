# Test fixture: a per-sample process() that produces NaN for loud input (plan 2.1).


class nan_samples:
    def process(self, x: float) -> float:
        if x > 0.75:
            return float("nan")
        return x
