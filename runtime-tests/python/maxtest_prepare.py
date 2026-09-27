# Runtime-test fixture (runtime-tests/run.py copies it into python/): remembers what prepare()
# was told, and outputs it on request: the input selects what — 1 the sample rate, 2 the vector
# size, 3 how many times prepare() ran, 4 how many samples process() saw before it first ran.


class maxtest_prepare:
    def __init__(self):
        self._sample_rate = 0.0
        self._vector_size = 0
        self._prepares = 0
        self._early = 0

    def prepare(self, sample_rate: float, vector_size: int) -> None:
        self._sample_rate = sample_rate
        self._vector_size = vector_size
        self._prepares += 1

    def process(self, x: float) -> float:
        if self._prepares == 0:
            self._early += 1
        if x == 1.0:
            return self._sample_rate
        if x == 2.0:
            return float(self._vector_size)
        if x == 3.0:
            return float(self._prepares)
        if x == 4.0:
            return float(self._early)
        return 0.0
