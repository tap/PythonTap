# Test fixture: user code that tries to quit the host process (plan 1.1).
import sys


class exits:
    level: float = 1.0

    def quit(self) -> None:
        sys.exit(3)

    def raise_system_exit(self) -> None:
        raise SystemExit("raised directly")

    def process(self, x: float) -> float:
        if x > 0.75:
            sys.exit("from process()")
        return x

    def __setattr__(self, name, value):
        if name == "level" and value < 0:
            sys.exit("from an attribute setter")
        object.__setattr__(self, name, value)
