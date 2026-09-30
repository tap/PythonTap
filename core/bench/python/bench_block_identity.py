# Benchmark fixture (plan 6.3): the block path with no work in it — what the bridge costs.
import numpy as np


class bench_block_identity:
    def process(self, x: np.ndarray) -> np.ndarray:
        return x
