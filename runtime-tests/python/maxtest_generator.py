# Runtime-test fixture (runtime-tests/run.py copies it into python/): process() with no inputs —
# a generator (plan 2.4). The object still has one inlet, for messages.


class maxtest_generator:
    level: float = 0.25

    def process(self) -> float:
        return self.level
