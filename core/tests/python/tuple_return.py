# Test fixture: process() declares a tuple return, which is not supported yet.


class tuple_return:
    def process(self, x: float) -> tuple:
        return (x, x)
