# Test fixture: every shape of hint and signature the bridge handles (plan 3.1–3.3).
from typing import ClassVar, Optional


class typed:
    flag: bool = False
    gain: Optional[float] = 1.0
    count: int | None = 3
    label: str = "x"
    shared: ClassVar[int] = 7          # a class variable, not a field
    made: str = ""

    def set_flag(self, on: bool) -> None:
        self.flag = on
        self.label = type(on).__name__

    def scale(self, x: float, y: float = 2.0) -> None:
        self.gain = x * y

    def many(self, *values: float) -> None:
        self.gain = sum(values)

    def untyped(self, a, b) -> None:
        self.label = f"{type(a).__name__}:{type(b).__name__}"

    def kwonly_ok(self, a: int, *, k: int = 1) -> None:
        self.count = a + k

    def kwonly_required(self, a: int, *, k: int) -> None:
        pass

    def clear_gain(self) -> None:
        self.gain = None

    def odd_values(self) -> None:
        self.count = 2.7
        self.label = 5

    @classmethod
    def make(cls, what: str) -> None:
        cls.made = what

    @staticmethod
    def helper(v: float) -> None:
        typed.made = f"helper {v}"

    @property
    def expensive(self):
        raise RuntimeError("a property getter ran while the class was being described")

    def process(self, x: float) -> float:
        return x * (self.gain or 0.0)
