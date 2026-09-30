# Test fixture: process(*args), which cannot say how many inputs it has (plan 2.4).


class star_inputs:
    def process(self, *xs: float) -> float:
        return sum(xs)
