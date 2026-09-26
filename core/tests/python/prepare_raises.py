# Test fixture: a prepare() that raises (plan 2.3).


class prepare_raises:
    def prepare(self, sample_rate: float, vector_size: int) -> None:
        raise RuntimeError("prepare() failed on purpose")

    def process(self, x: float) -> float:
        return x
