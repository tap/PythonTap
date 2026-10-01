# Test fixture: a process() that never returns while `forever` is set (plan 8.3). The loop swallows
# every Exception, so only a BaseException — the worker's WorkerStopped — can end it from outside.


class hangs:
    forever: bool = True

    def process(self, x: float) -> float:
        while self.forever:
            try:
                x = x * 1.0
            except Exception:
                pass
        return x
