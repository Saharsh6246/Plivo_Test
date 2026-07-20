# Experiment Log

All experiments ran for 30 seconds (1,500 frames) with the 4-data/3-parity FEC transport.

| Profile | Seed | Delay (ms) | Misses | Miss rate | Overhead | Result |
|---|---:|---:|---:|---:|---:|---|
| A_mild | 1 | 160 | 0 | 0.00% | 1.81x | VALID |
| A_mild | 5 | 130 | 0 | 0.00% | 1.81x | VALID |
| B_moderate | 1 | 180 | 4 | 0.27% | 1.81x | VALID |
| B_moderate | 1 | 170 | 4 | 0.27% | 1.81x | VALID |
| B_moderate | 1 | 160 | 4 | 0.27% | 1.81x | VALID |
| B_moderate | 1 | 150 | 4 | 0.27% | 1.81x | VALID |
| B_moderate | 1 | 140 | 5 | 0.33% | 1.81x | VALID |
| B_moderate | 1 | 130 | 7 | 0.47% | 1.81x | VALID |
| B_moderate | 1 | 120 | 22 | 1.47% | 1.81x | INVALID |
| B_moderate | 2 | 130 | 4 | 0.27% | 1.81x | VALID |
| B_moderate | 3 | 130 | 2 | 0.13% | 1.81x | VALID |
| B_moderate | 4 | 130 | 5 | 0.33% | 1.81x | VALID |
| B_moderate | 5 | 130 | 5 | 0.33% | 1.81x | VALID |


## Causal parity transport (final)

The block-FEC transport was replaced with a causal XOR parity stream, which sends parity with the protected frame and removes block-formation latency.

| Profile | Seed | Delay (ms) | Misses | Miss rate | Overhead | Result |
|---|---:|---:|---:|---:|---:|---|
| A_mild | 1 | 80 | 0 | 0.00% | 1.997x | VALID |
| B_moderate | 1 | 80 | 8 | 0.53% | 1.997x | VALID |
| B_moderate | 2 | 80 | 11 | 0.73% | 1.997x | VALID |
| B_moderate | 3 | 80 | 11 | 0.73% | 1.997x | VALID |
| B_moderate | 4 | 80 | 3 | 0.20% | 1.997x | VALID |
| B_moderate | 5 | 80 | 12 | 0.80% | 1.997x | VALID |

## Safety validation at 90 ms

| Profile | Seed | Delay (ms) | Misses | Miss rate | Overhead | Result |
|---|---:|---:|---:|---:|---:|---|
| B_moderate | 1 | 90 | 3 | 0.20% | 1.997x | VALID |
| B_moderate | 2 | 90 | 1 | 0.07% | 1.997x | VALID |
| B_moderate | 3 | 90 | 4 | 0.27% | 1.997x | VALID |
| B_moderate | 4 | 90 | 1 | 0.07% | 1.997x | VALID |
| B_moderate | 5 | 90 | 5 | 0.33% | 1.997x | VALID |

The selected grading delay is 90 ms. It leaves 10 ms of scheduling margin above Profile B's measured 80 ms one-way jitter and remained below both mandatory caps across the seed sweep.
