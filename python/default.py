from attrs import define, field


@define
class default:
    """The class loaded by tap.python~ when no source argument is given.

    Demonstrates the mapping from Python to Max:
    - the annotated 'gain' field becomes a Max attribute
    - any public method (like 'greet') becomes a Max message
    - methods named 'int' and 'float' answer those standard Max messages
    - process() runs on the audio signal, once per sample (see numpy_gain.py
      for the much cheaper once-per-vector form)
    """

    gain: float = field(default = 1.0)

    def greet(self, name: str) -> None:
        print(f"hello {name}, from python!")

    def process(self, x: float) -> float:
        return x * self.gain

    # These two come last on purpose: once `def float` has run, the name `float` in
    # the class body means this method, so an annotation written after it would no
    # longer mean the built-in type. (Inside method bodies the built-ins are unaffected.)

    def int(self, new_gain: int) -> None:
        self.gain = float(new_gain)

    def float(self, new_gain: float) -> None:
        self.gain = new_gain
