import numpy as np
from attrs import define, field


@define
class allpass:
    delay:  float   = field(default = 0.0)
    alpha:  float   = field(default = 0.0)
    fs:     int     = field(default = 48000)

    _delay_in_samples:  int         = field(init = False, default = 0)
    _x:                 np.ndarray  = field(init = False, default = np.zeros(2))
    _y:                 np.ndarray  = field(init = False, default = np.zeros(2))
    _write_index:       int         = field(init = False, default = 0)
    _read_index:        int         = field(init = False, default = 1)


    @delay.validator
    def _set_delay(self, attribute, value):
        self._update_delay(value, self.fs)


    @alpha.validator
    def _check_alpha(self, attribute, value):
        if value < -1.0 or value > 1.0:
            raise ValueError("alpha must be in the range [-1.0, 1.0]")

    
    @fs.validator
    def _set_fs(self, attribute, value):
        self._update_delay(self.delay, value)


    def _update_delay(self, delay_in_ms, sampling_frequency):
        new_delay_in_samples = int((delay_in_ms / 1000.0) * self.fs)
        if new_delay_in_samples != self._delay_in_samples:
            self._delay_in_samples = new_delay_in_samples
            print(f"Setting Delay to {delay_in_ms} ms ({self._delay_in_samples} samples @ fs={sampling_frequency})")
            self.clear()


    def clear(self) -> None:
        print("zeroing history to reset allpass filter")
        self._x = np.zeros(self._delay_in_samples)
        self._y = np.zeros(self._delay_in_samples)
        self._write_index = 0
        self._read_index = 1


    def process(self, input: float) -> float:
        x = input
        x1 = self.x[m_read_index]
        y1 = self.y[m_read_index]

        y = x1 + ((x - y1) * self.attr_alpha)
            
        self.x[m_write_index] = x
        self.y[m_write_index] = y

        m_read_index += 1
        if m_read_index == self.m_delay_in_samples: m_read_index = 0 
        m_write_index += 1
        if m_write_index == self.m_delay_in_samples: m_write_index = 0 

        return y

