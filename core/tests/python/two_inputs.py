# Test fixture: process() with two inputs and one output (plan 2.4).


class two_inputs:
    def process(self, x: float, y: float) -> float:
        return x - y
