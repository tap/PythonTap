# Runtime-test fixture (runtime-tests/run.py copies it into python/): edits the other fixtures'
# files, as a person would in an editor, so that the object's file watcher reloads them.
import re
from pathlib import Path


class maxtest_editor:
    def __init__(self):
        self._saved = {}

    def _file(self, name: str) -> Path:
        return Path(__file__).with_name(f"{name}.py")

    def scale(self, name: str, value: float) -> None:
        path = self._file(name)
        source = re.sub(r"^SCALE = .*$", f"SCALE = {value!r}", path.read_text(encoding="utf-8"), count=1, flags=re.M)
        path.write_text(source, encoding="utf-8")

    def bump(self, name: str) -> None:
        """Save the file with its REVISION one higher: a real change, so the module runs again."""
        path = self._file(name)
        source = path.read_text(encoding="utf-8")
        revision = int(re.search(r"^REVISION = (\d+)$", source, flags=re.M).group(1))
        source = re.sub(r"^REVISION = .*$", f"REVISION = {revision + 1}", source, count=1, flags=re.M)
        path.write_text(source, encoding="utf-8")

    def reshape(self, name: str, inputs: int, outputs: int) -> None:
        """Save the file as a class with `inputs` inputs and `outputs` outputs, each output passing the
        input in its position (or silence): process() changes shape (plan 2.4)."""
        parameters = "".join(f", x{i}: float" for i in range(inputs))
        values = [f"x{i}" if i < inputs else "0.0" for i in range(outputs)]
        returns = "float" if outputs == 1 else "tuple[" + ", ".join(["float"] * outputs) + "]"
        result = values[0] if outputs == 1 else "(" + ", ".join(values) + ")"
        source = f"class {name}:\n    def process(self{parameters}) -> {returns}:\n        return {result}\n"
        self._file(name).write_text(source, encoding="utf-8")

    def remove_field(self, name: str, field: str) -> None:
        path = self._file(name)
        source = re.sub(rf"^\s+{field}: .*\n", "", path.read_text(encoding="utf-8"), count=1, flags=re.M)
        path.write_text(source, encoding="utf-8")

    def corrupt(self, name: str) -> None:
        path = self._file(name)
        self._saved[name] = path.read_text(encoding="utf-8")
        path.write_text(self._saved[name] + "\ndef (\n", encoding="utf-8")

    def restore(self, name: str) -> None:
        self._file(name).write_text(self._saved.pop(name), encoding="utf-8")

    def process(self, x: float) -> float:
        return 0.0
