# Test fixture: no inputs — a generator (plan 2.4): the sample count so far.


class generator:
    def __init__(self):
        self.count = 0

    def process(self) -> float:
        self.count += 1
        return float(self.count)
