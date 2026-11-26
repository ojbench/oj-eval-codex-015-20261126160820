import pathlib
import random
import subprocess
import tempfile
import time

binary = pathlib.Path(__file__).resolve().parents[1] / "code"
rng = random.Random(2545)
state = {}

with tempfile.TemporaryDirectory() as directory:
    def run(commands):
        expected = []
        for command in commands:
            parts = command.split()
            op, key = parts[:2]
            values = state.setdefault(key, set())
            if op == "insert":
                values.add(int(parts[2]))
            elif op == "delete":
                values.discard(int(parts[2]))
            else:
                expected.append(" ".join(map(str, sorted(values))) or "null")
        start = time.monotonic()
        result = subprocess.run(
            [str(binary)], cwd=directory,
            input=str(len(commands)) + "\n" + "\n".join(commands) + "\n",
            text=True, capture_output=True, check=True,
        )
        assert result.stdout.splitlines() == expected
        print(f"Passed {len(commands)} commands in {time.monotonic() - start:.3f}s")

    keys = ["a", "a" * 64, "b", "c", "z" * 64]
    commands = [f"insert {keys[i % 5]} {i}" for i in range(95000)]
    rng.shuffle(commands)
    run(commands + [f"find {key}" for key in keys])
    # Repeat processes against the same file to verify persisted tree metadata.
    for batch in range(3):
        commands = []
        for _ in range(25000):
            key = rng.choice(keys + ["missing"])
            op = rng.choices(["insert", "delete", "find"], [3, 6, 1])[0]
            value = rng.randrange(95000)
            commands.append(f"{op} {key}" + (f" {value}" if op != "find" else ""))
        run(commands)
    commands = [f"delete {key} {value}" for key, values in state.items() for value in values]
    run(commands + [f"find {key}" for key in keys])
    run([f"insert {key} {value}" for key in keys for value in [0, 2147483647]]
        + [f"find {key}" for key in keys])
