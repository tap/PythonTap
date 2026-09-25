# Test fixture: process() raises on its third call.


class raises_in_process:
    def __init__(self):
        self.calls = 0

    def process(self, x: float) -> float:
        self.calls += 1
        if self.calls >= 3:
            raise ValueError("process() failed on purpose")
        return x
