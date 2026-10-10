# PythonTap

Write Max objects in Python. PythonTap embeds a CPython interpreter in a Max package and runs a
Python class as a Max object, in one of two ways:

- **`tap.python~`** runs the class as an **audio object**: its `process()` method is called on the
  signal — once per signal vector with numpy arrays, or once per sample with floats.
- **`tap.python`** runs it as an **ordinary object, without audio**: messages in, messages out — and
  what a method returns is what the object outputs.

Either way:

- The class's **type-annotated attributes** become Max attributes (`@gain 0.5` in the object box,
  `gain 0.5`, `getgain`, attrui — it all works).
- The class's **public methods** become Max messages, called according to their signatures, with
  arguments converted according to their type hints.
- The source file is **watched and hot-reloaded** every time you save it, keeping attribute values,
  so you can live-code with Max running.
- Python's `print()` output and tracebacks land in the **Max console**, posted from Max's main thread
  whatever thread printed — and no exception your code raises, `sys.exit()` included, can take Max
  down.

```python
{{#include ../../python/default.py}}
```

```
[tap.python~ default]   ← loads python/default.py: a 'gain' attribute, process() on the signal
[tap.python]            ← the same file, without audio: a bang outputs the gain
```

The package comes with CPython 3.13, numpy and attrs, on macOS (Apple Silicon and Intel) and
Windows, for Max 9.

## This book

- [Installing](installing.md) the package, and adding Python packages to its runtime.
- [Writing a class](writing-a-class.md): the file, attributes, messages, hot reload — what is the same
  for both objects. Then [Audio](audio.md) and [Worker mode](worker-mode.md) for `tap.python~`, and
  [Objects without audio](control-objects.md) for `tap.python`.
- [Performance](performance.md), measured, and [Errors and limits](errors-and-limits.md): what the
  objects guard against and what they cannot.
- [The examples](examples.md) in the package's `python` folder, and [a notebook](notebook.md) that
  runs one of them the way the object does.
- For contributors: [Building](building.md), [Testing](testing.md) and [the design](design.md).

In Max itself, the package's guide *Writing Max Objects in Python* and three tutorials are in the
Documentation window (Package Docs › PythonTap), and *PythonTap Overview* is in the Extras menu.
