# Runtime-test fixture (runtime-tests/run.py copies it into python/): maxtest_editor edits this
# file while it runs — SCALE, the extra field, a syntax error — as a person would in an editor.

SCALE = 1.0


class maxtest_reload:
    gain: float = 1.0
    extra: float = 0.0

    def process(self, x: float) -> float:
        return x * self.gain * SCALE
