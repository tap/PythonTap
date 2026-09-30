# Runtime-test fixture (runtime-tests/run.py copies it into python/): a class the object must
# describe with a diagnostic — notify() is reserved by the object — which is true of the class, so
# said once per run of this file however many objects share it (plan 6.7). maxtest_editor bumps
# REVISION to change the file.

REVISION = 0


class maxtest_reserved:
    def notify(self) -> None:
        pass

    def process(self, x: float) -> float:
        return x
