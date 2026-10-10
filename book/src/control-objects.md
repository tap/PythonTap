# Objects without audio: `tap.python`

`[tap.python name]` runs a class as an ordinary Max object — messages in, messages out, no signal — with everything [Writing a class](writing-a-class.md) says but the audio: the same `python` folder and loader (one file can serve a `tap.python~` and a `tap.python` at once, executed once per save), the same attributes, messages, hot reload, console and error guards. And one thing more: **what a method returns is what the object outputs**.

```python
from attrs import define, field

@define
class euclid:
    steps: int = field(default = 8)
    pulses: int = field(default = 3)

    def bang(self) -> list[int]:
        steps = max(1, self.steps)
        return [int((i * self.pulses) % steps < self.pulses) for i in range(steps)]
```

```
[tap.python euclid]   ← a bang outputs 1 0 0 1 0 0 1 0; steps 16, @pulses 5 or an attrui change it
```

## Output

What a method returns goes out of the object before the message returns, converted by its type:

| The method returns | The object outputs |
|---|---|
| `None` | nothing |
| a `bool` (numpy's too) | an int, 0 or 1 |
| an `int` (anything with `__index__`: `np.int64`, an `IntEnum`) | an int (64-bit: a value past it is reported, and nothing is output) |
| a `float` (anything with `__float__`: `np.float32`) | a float — NaN and infinity as they are |
| a `str` | `symbol <s>`: one symbol, whatever it holds — `"hello world"` is one symbol, `"60"` the symbol and not the number |
| a `list`, a `range`, a 1-D `np.ndarray`, or a `tuple` from a method not hinted `-> tuple[…]` | a list, each element by the rows above — or, when the first element is a `str`, the message it names (`["note", 60, 100]` outputs `note 60 100`); an empty one outputs nothing |
| *n* values from a method hinted `-> tuple[…]` of *n* members | one value per outlet, right to left; `None` in a slot outputs nothing from that outlet |
| anything else — a `dict`, a `set`, `bytes`, a 2-D array, an object — or a list holding `None` or another list | nothing, and the console says why |

A `str` is output as `symbol <s>` because that is what reaches every receiver as the string it is: as the message it names, `"bang"`, `"int"` and `"list"` would not be data at all. `[sel C]` matches it; `[route C]` does not (put `[route symbol]` first), `[prepend]` keeps the word `symbol`, and a message box's `[set $1(` displays it. To output the message a string names, return it in a list — `["start"]`.

## Outlets

The object has as many outlets as the widest `tuple[…]` return hint among its methods, at least one, plus a dumpout at the right: `getsteps` outputs `steps 8` from it, as a Max object with attributes does. The hint is the one place a type hint changes what a value does: unhinted, a returned tuple is a list from the first outlet. A tuple of unsaid length (`tuple[int, ...]`) keeps one outlet and is output as a list, which the console says once. A save that changes how many outlets the class needs changes the object's outlets in place, keeping the patch cords of those that stay.

## Messages

As in `tap.python~`, called according to their signatures, with `int`, `float`, `symbol`, `bang` and `list` answering those standard messages. A method named `anything` answers every message the class has no method or attribute for, with the selector first — `def anything(self, selector: str, *args)`; without one, the object says it doesn't understand, as Max objects do. `process()` and `prepare()` are ordinary methods here: `process 0.5` calls the class's `process()` and outputs what it returns, a way to try a filter sample by sample. The names Max or the object handle themselves are `tap.python~`'s less the audio ones (`dsp64`, `mode`, `latency` and the rest are free), plus `dumpout`.

## Threads

A message runs on the thread it arrives on, as an ordinary Max object's does — Max's main thread, or its scheduler thread with Overdrive on — and its result is output before the message returns, so `trigger`, `metro` and the rest compose with it as with any object. The limits:
- A method holds the thread it runs on for as long as it takes: under Overdrive, a heavy method driven by a `metro` holds Max's scheduler meanwhile. Sent through `[deferlow]`, it runs on the main thread instead.
- With **Scheduler in Audio Interrupt** on (and audio running), the scheduler thread is the audio thread, so a message from a `metro` runs Python on the audio thread, where it waits for any other Python — a reload, another object's message — and can interrupt the audio. The object says so in the console, once per session; `[deferlow]` takes the work off the audio thread.
- What a method returns is the only output: a thread your class starts (`threading.Thread`) has no outlet to reach.
- A `dict` is not output as a Max dictionary yet.

## Performance

A message costs the conversion of its arguments in, your method, and the conversion of what it returns out, all on the thread the message came on; [the performance chapter](performance.md) has what the bridge itself costs.
