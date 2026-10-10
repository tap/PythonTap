# Runtime-test fixture (runtime-tests/run.py copies it into python/): maxtest_editor rewrites it with
# returns(), so that its method's return hint names one value or several — the object's outlets follow.


class maxtest_widen:
    def bang(self) -> int:
        return 1
