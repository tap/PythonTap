import numpy as np
from attrs import define, field


@define
class numpy_allpass:
    """The allpass filter of allpass.py, processed a whole signal vector at a time with numpy.

    y[n] = alpha * x[n] + x[n - D] - alpha * y[n - D], with D the delay in samples. The
    feedback reaches back D samples, so any stretch of up to D samples depends only on samples
    before it: each stretch is one numpy expression, and a vector is split only when the delay
    is shorter than it. The output is the same as allpass.py's, sample for sample, at a small
    fraction of the cost — compare the two in the PythonTap book's Performance chapter.
    """

    delay:  float   = field(default = 1.0)      # delay time in ms
    alpha:  float   = field(default = 0.5)      # allpass coefficient, strictly between -1 and 1

    _fs:                float       = field(init = False, default = 48000.0)  # set by prepare()
    _vector_size:       int         = field(init = False, default = 64)       # set by prepare()
    _delay_in_samples:  int         = field(init = False, default = 1)
    # the last D inputs and outputs, then room for one vector
    _x:                 np.ndarray  = field(init = False, factory = lambda: np.zeros(1))
    _y:                 np.ndarray  = field(init = False, factory = lambda: np.zeros(1))

    # As in allpass.py: validators also run when tap.python~ sets an attribute, and during
    # __init__ they fire before the private fields exist, hence the hasattr guard.

    @delay.validator
    def _delay_changed(self, attribute, value):
        if value < 0.0:
            raise ValueError("delay must be non-negative")
        if hasattr(self, "_delay_in_samples"):
            self._update_delay(value, self._fs)

    @alpha.validator
    def _check_alpha(self, attribute, value):
        # |alpha| = 1 puts the feedback pole on the unit circle: the filter no longer decays
        if not -1.0 < value < 1.0:
            raise ValueError("alpha must be strictly between -1.0 and 1.0")

    def __attrs_post_init__(self):
        self._update_delay(self.delay, self._fs)
        self.clear()

    def prepare(self, sample_rate: float, vector_size: int) -> None:
        """Called by tap.python~ with Max's audio settings, before audio and whenever they change."""
        self._fs = sample_rate
        if vector_size != self._vector_size:
            self._vector_size = vector_size
            self.clear()
        self._update_delay(self.delay, sample_rate)

    def _update_delay(self, delay_in_ms, sampling_frequency):
        new_delay_in_samples = max(1, int((delay_in_ms / 1000.0) * sampling_frequency))
        if new_delay_in_samples != self._delay_in_samples:
            self._delay_in_samples = new_delay_in_samples
            print(f"Setting delay to {delay_in_ms} ms ({new_delay_in_samples} samples @ {sampling_frequency:g} Hz)")
            self.clear()

    def clear(self) -> None:
        self._x = np.zeros(self._delay_in_samples + self._vector_size)
        self._y = np.zeros(self._delay_in_samples + self._vector_size)

    def process(self, x: np.ndarray) -> np.ndarray:
        d, n = self._delay_in_samples, len(x)
        if d + n > len(self._x):  # a longer vector than prepare() announced: make room, keeping the history
            self._x = np.concatenate((self._x[:d], np.zeros(n)))
            self._y = np.concatenate((self._y[:d], np.zeros(n)))
        xs, ys, alpha = self._x, self._y, self.alpha
        xs[d:d + n] = x
        for start in range(0, n, d):  # at most d samples at a time: each y[n - d] is already computed
            end = min(start + d, n)
            # written as allpass.py computes it, so the two agree to the last bit
            ys[d + start:d + end] = xs[start:end] + (xs[d + start:d + end] - ys[start:end]) * alpha
        y = ys[d:d + n].copy()
        xs[:d] = xs[n:n + d]  # keep the last d inputs and outputs for the next vector
        ys[:d] = ys[n:n + d]
        return y
