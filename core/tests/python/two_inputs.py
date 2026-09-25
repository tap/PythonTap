# Test fixture: process() declares two inputs; only the first is supported.


class two_inputs:
    def process(self, x: float, y: float) -> float:
        return x
