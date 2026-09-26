# Test fixture: a class with a prepare() hook (plan 2.3). process() outputs the sample rate, so a
# vector processed before prepare() ran would show up as 0.0.


class prepared:
    sample_rate: float = 0.0
    vector_size: int = 0
    prepare_calls: int = 0

    def prepare(self, sample_rate: float, vector_size: int) -> None:
        self.sample_rate = sample_rate
        self.vector_size = vector_size
        self.prepare_calls += 1

    def process(self, x: float) -> float:
        return self.sample_rate
