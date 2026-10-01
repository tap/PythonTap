# Test fixture: a process() blocked in a call Python cannot interrupt — time.sleep delivers an
# asynchronous exception only when it returns (plan 8.3). Identity once `seconds` is 0.
import time


class sleeps:
    seconds: float = 3.0

    def process(self, x: float) -> float:
        if self.seconds:
            time.sleep(self.seconds)
        return x
