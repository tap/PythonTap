from attrs import define, field

@define
class python_thru:
    gain: float = field(default = 1.0)

    def foo(self, arg1: float) -> float:
        print("foo foo on yo do do")
        print(f"arg: {arg1}")
        return arg1 + 1

    def process(self, x: float) -> float:
        y = x * self.gain
        return y
