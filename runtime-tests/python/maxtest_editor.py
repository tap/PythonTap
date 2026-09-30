# Runtime-test fixture (runtime-tests/run.py copies it into python/): edits the other fixtures'
# files, as a person would in an editor, so that the object's file watcher reloads them. (The
# encoding is explicit: the embedded interpreter's default is ASCII, not UTF-8.)
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
