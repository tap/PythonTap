import numpy as np
from attrs import define, field


@define
class allpass:
    """First-order allpass filter with a delay line, for use with tap.python~."""

    delay:  float   = field(default = 1.0)      # delay time in ms
    alpha:  float   = field(default = 0.5)      # allpass coefficient
    fs:     int     = field(default = 48000)    # sampling frequency

    _delay_in_samples:  int         = field(init = False, default = 1)
    _x:                 np.ndarray  = field(init = False, factory = lambda: np.zeros(1))
    _y:                 np.ndarray  = field(init = False, factory = lambda: np.zeros(1))
    _write_index:       int         = field(init = False, default = 0)
    _read_index:        int         = field(init = False, default = 0)

    # Validators also run on plain attribute assignment, which is how tap.python~
    # sets attributes from Max. During __init__ they fire before the private
    # fields exist, so guard with hasattr and do the initial setup in post-init.

    @delay.validator
    def _delay_changed(self, attribute, value):
        if value < 0.0:
            raise ValueError("delay must be non-negative")
        if hasattr(self, "_delay_in_samples"):
            self._update_delay(value, self.fs)

    @alpha.validator
    def _check_alpha(self, attribute, value):
        if value < -1.0 or value > 1.0:
            raise ValueError("alpha must be in the range [-1.0, 1.0]")

    @fs.validator
    def _fs_changed(self, attribute, value):
        if value <= 0:
            raise ValueError("fs must be positive")
        if hasattr(self, "_delay_in_samples"):
            self._update_delay(self.delay, value)

    def __attrs_post_init__(self):
        self._update_delay(self.delay, self.fs)
        self.clear()

    def _update_delay(self, delay_in_ms, sampling_frequency):
        new_delay_in_samples = max(1, int((delay_in_ms / 1000.0) * sampling_frequency))
        if new_delay_in_samples != self._delay_in_samples:
            self._delay_in_samples = new_delay_in_samples
            print(f"Setting delay to {delay_in_ms} ms ({new_delay_in_samples} samples @ fs={sampling_frequency})")
            self.clear()

    def clear(self) -> None:
        self._x = np.zeros(self._delay_in_samples)
        self._y = np.zeros(self._delay_in_samples)
        # process() reads before it writes, so reading the slot about to be
        # overwritten yields the value written exactly _delay_in_samples ago
        self._write_index = 0
        self._read_index = 0

    def process(self, x: float) -> float:
        x1 = self._x[self._read_index]
        y1 = self._y[self._read_index]

        y = x1 + (x - y1) * self.alpha

        self._x[self._write_index] = x
        self._y[self._write_index] = y

        self._read_index = (self._read_index + 1) % self._delay_in_samples
        self._write_index = (self._write_index + 1) % self._delay_in_samples

        return y
