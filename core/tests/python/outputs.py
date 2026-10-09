# Test fixture: what a method returns, and what tap.python outputs for it (plan 9.1). Each method
# returns the value CASES names; numpy's cases are there when numpy is.
import enum

try:
    import numpy as np
except ImportError:
    np = None


class Level(enum.IntEnum):
    HIGH = 3


CASES = {
    "none": lambda: None,
    "true": lambda: True,
    "false": lambda: False,
    "int": lambda: 60,
    "negative": lambda: -5,
    "int_max": lambda: 2**63 - 1,
    "int_min": lambda: -(2**63),
    "int_over": lambda: 2**63,
    "int_under": lambda: -(2**63) - 1,
    "int_enum": lambda: Level.HIGH,
    "float": lambda: 1.5,
    "nan": lambda: float("nan"),
    "inf": lambda: float("inf"),
    "str": lambda: "C",
    "empty_str": lambda: "",
    "digits": lambda: "60",
    "spaced": lambda: "hello world",
    "list": lambda: [1, 2.5, "x"],
    "message": lambda: ["note", 60, 100],
    "one_word": lambda: ["start"],
    "empty_list": lambda: [],
    "tuple": lambda: (1, 2),
    "range": lambda: range(3),
    "bools": lambda: [True, False],
    "holding_none": lambda: [1, None],
    "nested": lambda: [1, [2]],
    "big_in_list": lambda: [2**64],
    "dict": lambda: {"a": 1},
    "set": lambda: {1},
    "bytes": lambda: b"ab",
    "generator": lambda: (i for i in range(2)),
    "object": lambda: object(),
    "pair": lambda: (60, "C"),
    "list_pair": lambda: [60, "C"],
    "none_slot": lambda: (None, "x"),
    "all_none": lambda: (None, None),
    "three": lambda: (1, 2, 3),
    "slot_dict": lambda: (1, {}),
    "slot_list": lambda: ([1, 2], "x"),
    "one": lambda: (5,),
}
if np is not None:
    CASES.update({
        "np_bool": lambda: np.bool_(True),
        "np_int64": lambda: np.int64(7),
        "np_uint8": lambda: np.uint8(200),
        "np_uint64_max": lambda: np.uint64(2**64 - 1),
        "np_float32": lambda: np.float32(0.5),
        "np_float64": lambda: np.float64(0.25),
        "np_nan": lambda: np.float64("nan"),
        "np_array": lambda: np.array([1.0, 2.0]),
        "np_int_array": lambda: np.array([1, 2]),
        "np_bool_array": lambda: np.array([True, False]),
        "np_str_array": lambda: np.array(["a", "b"]),
        "np_0d": lambda: np.array(5.0),
        "np_0d_int": lambda: np.array(3),
        "np_2d": lambda: np.array([[1.0, 2.0]]),
        "np_empty": lambda: np.array([]),
        "np_in_list": lambda: [np.float32(0.5), np.int16(3), np.bool_(False)],
        "array_in_list": lambda: [np.array([1.0])],
    })


class outputs:
    def give(self, case: str):
        return CASES[case]()

    def two(self, case: str) -> tuple[int, str]:
        return CASES[case]()

    def one(self, case: str) -> tuple[int]:
        return CASES[case]()

    def unsaid(self, case: str) -> tuple[int, ...]:
        return CASES[case]()

    def boom(self) -> int:
        raise ValueError("boom() raised on purpose")
