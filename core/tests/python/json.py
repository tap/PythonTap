# Test fixture: a user file named like a standard-library module (plan 3.5). It is loaded by path
# for [tap.python~ json], while `import json` anywhere still gets the standard library.


class json:
    def process(self, x: float) -> float:
        return 7.0
