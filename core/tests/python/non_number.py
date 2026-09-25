# Test fixture: process() returns something that is not a number.


class non_number:
    def process(self, x: float) -> float:
        return "not a number"
