# Test fixture: one file loaded by an audio object and an object without audio (plan 9.1, audit
# M5): each kind is told, once, what is true of the class as it binds it.


class kinds:
    def dsp(self) -> None:  # reserved by the audio host in the tests
        pass

    def spread(self) -> tuple[int, ...]:  # a tuple of unsaid length: a list, said once by tap.python
        return 1, 2

    def process(self, x: float) -> float:
        return x
