# Benchmark fixture (plan 9.1): a tap.python message that returns its list as it came — what the
# bridge costs a message: 64 atoms converted in, the list converted back out.


class bench_list_identity:
    def list(self, values: list[float]) -> list[float]:
        return values
