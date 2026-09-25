# Test fixture: a class with an attribute and a message but no process().


class no_process:
    level: float = 0.5

    def poke(self) -> None:
        pass
