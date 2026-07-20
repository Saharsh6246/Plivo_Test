# Plivo Systems Assignment: The Flaky Network

## Submission contents

- `sender.c` and `receiver.c`: the FEC sender and receiver.
- `Makefile`: builds `./sender` and `./receiver` with `make`.
- `SUMMARY.html`: system-level architecture and design explanation.
- `RUNLOG.md`: measured experiments and outcomes.
- `NOTES.md`: concise design notes and the recommended grading delay.

## Build and run

```bash
make clean && make
python3 run.py --profile profiles/B.json --delay_ms 130 --seed 1
```

The recommended grading delay is **130 ms**.
