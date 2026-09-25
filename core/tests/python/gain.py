# Test fixture for the core battery: a plain class (no attrs) exercising every mapping.


class gain:
    level: float = 1.0
    count: int = 0
    label: str = "unity"
    _private: float = 3.0

    def process(self, x: float) -> float:
        return x * self.level

    def set_level(self, value: float) -> None:
        self.level = value

    def bump(self, amount: int) -> None:
        self.count += amount

    def rename(self, name: str) -> None:
        self.label = name

    def pair(self, a: int, b: float) -> None:
        self.count = a
        self.level = b

    def say(self, text: str) -> None:
        print(f"say: {text}")

    def fail(self) -> None:
        raise RuntimeError("fail() was called")

    def _hidden(self) -> None:
        pass
