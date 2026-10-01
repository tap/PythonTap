# Test fixture: a process() that can be told to stall once, as a worker falling behind (plan 2.5).
# Identity otherwise, so each output sample says which input it came from.
import time


class stalls:
    stall: float = 0.0  # seconds to sleep at the next sample, once

    def process(self, x: float) -> float:
        if self.stall:
            seconds, self.stall = self.stall, 0.0
            time.sleep(seconds)
        return x
