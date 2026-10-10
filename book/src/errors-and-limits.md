# Errors and limits

An exception raised by your code (in `process()`, a message, an attribute setter, the constructor or at import) prints its traceback to the Max console and never takes Max down. That includes `sys.exit()`, which is reported like any other exception rather than quitting Max. What the object cannot catch is what never reaches Python's exception machinery: a process exit below it (`os._exit()`, `os.abort()`), a crash in a C extension (a broken wheel, `ctypes`), or code that never returns — an endless loop in a message or the constructor freezes Max's main thread, and in `process()` it stalls the audio thread (in worker mode, only the worker thread, which is interrupted or abandoned when the worker stops — see [Worker mode](worker-mode.md)). Another Max external that embeds its own CPython alongside this one is untested and unsupported: two interpreters in one Max share process-wide state neither expects to.

In `tap.python`, a method that raises outputs nothing: its traceback is printed instead. A file that
fails to load leaves a `tap.python` outputting nothing, and a `tap.python~` outputting silence, until a
save fixes it.
