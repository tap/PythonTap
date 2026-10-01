# Test fixture: annotated fields named like attributes the host reserves (plan 2.5: worker mode's
# mode and latency).


class reserved_field:
    mode: str = "fast"
    latency: int = 3
    level: float = 1.0

    def process(self, x: float) -> float:
        return x * self.level
