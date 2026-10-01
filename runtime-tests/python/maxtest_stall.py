# Runtime-test fixture (runtime-tests/run.py copies it into python/): passes its input through, but
# can be told to stall once — a worker falling behind (plan 2.5).
import time


class maxtest_stall:
    stall: float = 0.0  # seconds to sleep at the next sample, once

    def process(self, x: float) -> float:
        if self.stall:
            seconds, self.stall = self.stall, 0.0
            time.sleep(seconds)
        return x
