# Test fixture: the class cannot be instantiated.


class bad_constructor:
    def __init__(self):
        raise RuntimeError("constructor failed on purpose")

    def process(self, x: float) -> float:
        return x
