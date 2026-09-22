# HIL Test Report

| Metric | Value |
|---|---:|
| Steps | 20 |
| Passed | 20 |
| Failed | 0 |
| Skipped | 0 |
| Errors | 0 |

| Case | Line | Step | Target | Expected | Actual | Status | Message |
|---|---:|---|---|---|---|---|---|
| comm_timeout_and_power_off | 2 | SET | throttle_position | 20 | 20 | PASS | signal set |
| comm_timeout_and_power_off | 3 | WAIT |  | 300 ms | 300 ms | PASS | wait completed |
| comm_timeout_and_power_off | 4 | COMM_FAULT |  | COMM_FAULT ON | FAULT | PASS | communication fault updated |
| comm_timeout_and_power_off | 5 | WAIT |  | 100 ms | 400 ms | PASS | wait completed |
| comm_timeout_and_power_off | 6 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| comm_timeout_and_power_off | 7 | CLEAR_FAULT |  | CLEAR_FAULT | RECOVERY | PASS | fault cleared |
| comm_timeout_and_power_off | 8 | WAIT |  | 250 ms | 650 ms | PASS | wait completed |
| comm_timeout_and_power_off | 9 | ASSERT | engine_speed | GE 800.000000 | 800.000000 | PASS | assertion passed |
| comm_timeout_and_power_off | 10 | POWER_OFF |  | POWER_OFF | POWER_OFF | PASS | ECU powered off |
| comm_timeout_and_power_off | 11 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| comm_timeout_and_power_off | 12 | END |  |  |  | PASS | test case ended |
| latched_fault | 15 | SET | throttle_position | 20 | 20 | PASS | signal set |
| latched_fault | 16 | WAIT |  | 300 ms | 950 ms | PASS | wait completed |
| latched_fault | 17 | LATCH_FAULT |  | LATCH_FAULT | FAULT | PASS | fault latched |
| latched_fault | 18 | WAIT |  | 300 ms | 1250 ms | PASS | wait completed |
| latched_fault | 19 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| latched_fault | 20 | CLEAR_FAULT |  | CLEAR_FAULT | RECOVERY | PASS | fault cleared |
| latched_fault | 21 | WAIT |  | 250 ms | 1500 ms | PASS | wait completed |
| latched_fault | 22 | ASSERT | engine_speed | GE 800.000000 | 800.000000 | PASS | assertion passed |
| latched_fault | 23 | END |  |  |  | PASS | test case ended |
