# HIL Test Report

| Metric | Value |
|---|---:|
| Steps | 14 |
| Passed | 14 |
| Failed | 0 |
| Skipped | 0 |
| Errors | 0 |

| Case | Line | Step | Target | Expected | Actual | Status | Message |
|---|---:|---|---|---|---|---|---|
| power_up_and_set | 2 | SET | throttle_position | 25 | 25 | PASS | signal set |
| power_up_and_set | 3 | WAIT |  | 300 ms | 300 ms | PASS | wait completed |
| power_up_and_set | 4 | ASSERT | engine_speed | GT 800.000000 | 2350.000000 | PASS | assertion passed |
| power_up_and_set | 5 | ASSERT | battery_voltage | EQ 12.500000 | 12.500000 | PASS | assertion passed |
| power_up_and_set | 6 | END |  |  |  | PASS | test case ended |
| fault_stuck_and_recover | 9 | SET | throttle_position | 30 | 30 | PASS | signal set |
| fault_stuck_and_recover | 10 | WAIT |  | 300 ms | 600 ms | PASS | wait completed |
| fault_stuck_and_recover | 11 | FAULT | engine_speed | engine_speed STUCK 2000 | active from 600 ms | PASS | fault injected |
| fault_stuck_and_recover | 12 | WAIT |  | 20 ms | 620 ms | PASS | wait completed |
| fault_stuck_and_recover | 13 | ASSERT | engine_speed | EQ 2000.000000 | 2000.000000 | PASS | assertion passed |
| fault_stuck_and_recover | 14 | WAIT |  | 120 ms | 740 ms | PASS | wait completed |
| fault_stuck_and_recover | 15 | ASSERT | engine_speed | GT 800.000000 | 2660.000000 | PASS | assertion passed |
| fault_stuck_and_recover | 16 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| fault_stuck_and_recover | 17 | END |  |  |  | PASS | test case ended |
