# Runtime-test fixture (runtime-tests/run.py copies it into python/): what tap.python reports and
# survives — sys.exit() is reported, not obeyed; an exception prints its traceback; neither outputs.
import sys


class maxtest_control_faults:
    def leave(self) -> int:
        sys.exit(3)

    def fail(self) -> int:
        raise ValueError("maxtest_control_faults failed on purpose")

    def ok(self) -> int:
        return 1
