# Runtime-test fixture (runtime-tests/run.py copies it into python/): for tap.python, returns what it is
# given — so a patch can check every output against its input, on whatever thread the message came.


class maxtest_echo:
    def int(self, n: int) -> int:
        return n
