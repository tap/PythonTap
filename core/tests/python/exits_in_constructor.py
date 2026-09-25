# Test fixture: the constructor calls sys.exit() (plan 1.1).
import sys


class exits_in_constructor:
    def __init__(self):
        sys.exit("from __init__")
