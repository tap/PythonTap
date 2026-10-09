from attrs import define, field


@define
class euclid:
    """A Euclidean rhythm, for tap.python: a bang outputs `steps` 0s and 1s with `pulses` of the 1s
    spread as evenly as they go — 8 steps and 3 pulses is 1 0 0 1 0 0 1 0 — a list to drive a step
    sequencer.

    @steps and @pulses are its attributes, so `steps 16` or an attrui changes the pattern; what
    bang() returns, a list of ints, is what the object outputs.
    """

    steps: int = field(default = 8)
    pulses: int = field(default = 3)

    def bang(self) -> list[int]:
        steps = max(1, self.steps)
        return [int((i * self.pulses) % steps < self.pulses) for i in range(steps)]
