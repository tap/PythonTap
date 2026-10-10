# Runtime-test fixture (runtime-tests/run.py copies it into python/): for tap.python, one method per row
# of the output table (docs/TAP-PYTHON-PLAN.md; the book's "Objects without audio"). spread() and
# partial() are hinted tuple[str, int], so the object has two value outlets.
import numpy as np


class maxtest_outputs:
    def nothing(self) -> None:
        return None

    def yes(self) -> bool:
        return True

    def numpy_no(self):
        return np.bool_(False)

    def integer(self) -> int:
        return 42

    def big(self) -> int:
        return 2 ** 70  # past 64 bits: reported, nothing output

    def real(self) -> float:
        return 0.5

    def text(self) -> str:
        return "hello world"

    def word(self) -> str:
        return "bang"  # a str is symbol <s>, never the message it names

    def numbers(self) -> list:
        return [1, 2.5, 3]

    def message(self) -> list:
        return ["note", 60, 100]

    def pair(self):
        return (7, 8)  # a tuple from a method not hinted to return one: a list

    def array(self) -> np.ndarray:
        return np.array([1.0, 2.0])

    def scalar(self):
        return np.float64(0.25)

    def empty(self) -> list:
        return []

    def mapping(self):
        return {"a": 1}  # reported, nothing output

    def nested(self):
        return [1, [2]]  # reported, nothing output

    def spread(self) -> tuple[str, int]:
        return "left", 9

    def partial(self) -> tuple[str, int]:
        return None, 5  # nothing from the first outlet
