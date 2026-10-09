from attrs import define

NAMES = ("C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B")


@define
class note_name:
    """A MIDI note number in, its name out, for tap.python: the pitch class from the left outlet and
    the octave from the right — 60 outputs C and 4, middle C being C4.

    The return hint tuple[str, int] gives the object two outlets, one value each, output right to
    left as Max objects do: the octave, then the name (as `symbol C`).
    """

    # `int` comes last on purpose: once `def int` has run, the name `int` in the class body means this
    # method, so an annotation written after it would no longer mean the built-in type. (Inside method
    # bodies the built-ins are unaffected.)

    def int(self, note: int) -> tuple[str, int]:
        return NAMES[note % 12], note // 12 - 1
