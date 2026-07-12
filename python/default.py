from attrs import define, field


@define
class default:
    """The class loaded by tap.python~ when no source argument is given.

    Demonstrates the mapping from Python to Max:
    - the annotated 'gain' field becomes a Max attribute
    - 'float' and 'int' methods receive those standard Max messages
    - any other public method (like 'greet') becomes a Max message
    - process() runs on the audio signal, once per sample
    """

    gain: float = field(default = 1.0)

    def greet(self, name: str) -> None:
        print(f"hello {name}, from python!")

    def float(self, new_gain: float) -> None:
        self.gain = new_gain

    def int(self, new_gain: float) -> None:
        self.float(new_gain)

    def process(self, x: float) -> float:
        return x * self.gain
