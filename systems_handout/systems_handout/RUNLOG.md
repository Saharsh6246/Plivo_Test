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

The selected grading delay is 130 ms. It is the lowest tested valid delay and remained below both mandatory caps across the Profile B seed sweep.
