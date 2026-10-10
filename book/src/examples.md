# The examples

The package's `python` folder ships these classes. Each is the file itself, included here as it
ships; open one in a text editor beside Max, save a change, and the objects using it reload.

## For `tap.python~`

### `default.py` — a gain, per sample

What both objects load with no argument: a `gain` attribute, the messages `greet`, `int`, `float` and
`bang`, and `process()` once per sample. In `tap.python`, `process()` is a message like the others.

```python
{{#include ../../python/default.py}}
```

### `numpy_gain.py` — a gain, per vector

The same gain written for numpy: `process()` is hinted with arrays, so it runs once per signal
vector. See [Performance](performance.md) for what that saves.

```python
{{#include ../../python/numpy_gain.py}}
```

### `allpass.py` — a filter that knows the sample rate

A Schroeder allpass filter, per sample: `prepare()` turns its delay in milliseconds into samples, and
attrs validators guard its fields. [The notebook](notebook.md) runs it the way the object does.

```python
{{#include ../../python/allpass.py}}
```

### `numpy_allpass.py` — the same filter, per vector

The same filter a vector at a time — the core battery checks the two sample for sample.

```python
{{#include ../../python/numpy_allpass.py}}
```

### `stereo_width.py` — two inputs, two outputs

`process()`'s parameters are the object's signal inlets and its return hint its outlets.

```python
{{#include ../../python/stereo_width.py}}
```

## For `tap.python`

### `euclid.py` — a rhythm from a bang

```python
{{#include ../../python/euclid.py}}
```

### `scale.py` — a list through numpy

```python
{{#include ../../python/scale.py}}
```

### `note_name.py` — two outlets

```python
{{#include ../../python/note_name.py}}
```
