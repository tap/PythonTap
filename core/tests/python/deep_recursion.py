# Test fixture: recursion through a C boundary (sorted's key) to the recursion limit, on whatever
# thread calls process() — the worker's stack must take it (plan 8.3, audit A6). Returns its input.
import sys


class deep_recursion:
    def process(self, x: float) -> float:
        found = []

        def down(n: int) -> int:
            if n <= 0:
                found.append(x)
                return 0
            return sorted([n], key=lambda v: down(v - 1))[0]  # two Python frames and a C call per level

        down((sys.getrecursionlimit() - 100) // 2)
        return found[0]
