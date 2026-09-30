# Test fixture: process() returns a tuple without saying how many values (plan 2.4).


class tuple_return:
    def process(self, x: float) -> tuple:
        return (x, x)
