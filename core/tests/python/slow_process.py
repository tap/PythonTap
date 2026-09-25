# Test fixture: a process() slow enough that the GIL changes hands mid-vector (plan 1.2).


class slow_process:
    def process(self, x: float) -> float:
        total = 0
        for i in range(50):
            total += i
        return x
