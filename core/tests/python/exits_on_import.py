# Test fixture: the module calls sys.exit() while it is imported (plan 1.1).
import sys

sys.exit("from module top level")


class exits_on_import:
    pass
