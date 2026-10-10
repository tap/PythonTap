# Runtime-test fixture (runtime-tests/run.py copies it into python/): a tap.python class with an
# attribute, a method and an anything method — the forwarder's order, and attributes still set and
# read on a class that has anything.


class maxtest_forward:
    steps: int = 8

    def hello(self, n: int) -> int:
        return n + 1

    def anything(self, selector: str, *args) -> list:
        return [selector, *args]
