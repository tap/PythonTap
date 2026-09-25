# Test fixture: per-thread Python state on the audio thread (plan 1.4).
import threading

_local = threading.local()


class thread_state:
    def process(self, x: float) -> float:
        _local.count = getattr(_local, "count", 0) + 1
        return float(_local.count)
