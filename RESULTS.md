# Validation and Submission

- Solution: persistent B+ tree in `storage.db`, ordered by full key and integer value.
- Only pages needed by the current operation are read; no database cache is retained.
- Build: `cmake . && make`.
- Local validation: `python3 tests/check.py` passed randomized reference comparisons,
  repeated process restarts, full deletion, reinsertion, maximum-length keys,
  zero, and INT_MAX.

## Online Judge

- Problem: ACMOJ 2545.
- Submission: 879414 (attempt 1 of 7).
- Judged commit: `99f2f89f0046af0f507569523311247130456ea7`.
- Result: Accepted, all 29 test points across 9 groups.
- Score: 9/9.
- Reported peak memory: 4,325,376 bytes.
- Reported aggregate time: 909 ms; slowest test point: 376 ms.
- Remote branch hash was verified before submission. No repository changes or
  pushes were made while submission or judging was in progress.
