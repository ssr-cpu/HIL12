# HIL Test Report

| Metric | Value |
|---|---:|
| Steps | 38 |
| Passed | 38 |
| Failed | 0 |
| Skipped | 0 |
| Errors | 0 |

| Case | Line | Step | Target | Expected | Actual | Status | Message |
|---|---:|---|---|---|---|---|---|
| power_off_and_restart | 2 | SET | throttle_position | 20 | 20 | PASS | signal set |
| power_off_and_restart | 3 | WAIT |  | 300 ms | 300 ms | PASS | wait completed |
| power_off_and_restart | 4 | ASSERT | ecu_state | EQ 4.000000 | 4.000000 | PASS | assertion passed |
| power_off_and_restart | 5 | POWER | ecu | power OFF | POWER_OFF | PASS | power switched |
| power_off_and_restart | 6 | ASSERT | ecu_state | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| power_off_and_restart | 7 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| power_off_and_restart | 8 | POWER | ecu | power ON | INIT | PASS | power switched |
| power_off_and_restart | 9 | ASSERT | ecu_state | EQ 1.000000 | 1.000000 | PASS | assertion passed |
| power_off_and_restart | 10 | SET | throttle_position | 20 | 20 | PASS | signal set |
| power_off_and_restart | 11 | WAIT |  | 300 ms | 600 ms | PASS | wait completed |
| power_off_and_restart | 12 | ASSERT | ecu_state | EQ 4.000000 | 4.000000 | PASS | assertion passed |
| power_off_and_restart | 13 | ASSERT | engine_speed | GT 800.000000 | 2040.000000 | PASS | assertion passed |
| power_off_and_restart | 14 | END |  |  |  | PASS | test case ended |
| comm_timeout_recoverable | 17 | SET | throttle_position | 20 | 20 | PASS | signal set |
| comm_timeout_recoverable | 18 | WAIT |  | 300 ms | 900 ms | PASS | wait completed |
| comm_timeout_recoverable | 19 | ASSERT | ecu_state | EQ 4.000000 | 4.000000 | PASS | assertion passed |
| comm_timeout_recoverable | 20 | FAULT | ECU | ECU COMM | FAULT latched=0 | PASS | ECU fault injected |
| comm_timeout_recoverable | 21 | ASSERT | ecu_state | EQ 5.000000 | 5.000000 | PASS | assertion passed |
| comm_timeout_recoverable | 22 | WAIT |  | 20 ms | 920 ms | PASS | wait completed |
| comm_timeout_recoverable | 23 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| comm_timeout_recoverable | 24 | FAULT | ECU | ECU CLEAR | RECOVERY latched=0 | PASS | ECU fault injected |
| comm_timeout_recoverable | 25 | ASSERT | ecu_state | EQ 6.000000 | 6.000000 | PASS | assertion passed |
| comm_timeout_recoverable | 26 | WAIT |  | 300 ms | 1220 ms | PASS | wait completed |
| comm_timeout_recoverable | 27 | ASSERT | ecu_state | EQ 4.000000 | 4.000000 | PASS | assertion passed |
| comm_timeout_recoverable | 28 | END |  |  |  | PASS | test case ended |
| ecu_latched_fault | 31 | SET | throttle_position | 20 | 20 | PASS | signal set |
| ecu_latched_fault | 32 | WAIT |  | 300 ms | 1520 ms | PASS | wait completed |
| ecu_latched_fault | 33 | ASSERT | ecu_state | EQ 4.000000 | 4.000000 | PASS | assertion passed |
| ecu_latched_fault | 34 | FAULT | ECU | ECU LATCH | FAULT latched=1 | PASS | ECU fault injected |
| ecu_latched_fault | 35 | ASSERT | ecu_state | EQ 5.000000 | 5.000000 | PASS | assertion passed |
| ecu_latched_fault | 36 | ASSERT | ecu_fault_latched | EQ 1.000000 | 1.000000 | PASS | assertion passed |
| ecu_latched_fault | 37 | WAIT |  | 1000 ms | 2520 ms | PASS | wait completed |
| ecu_latched_fault | 38 | ASSERT | ecu_state | EQ 5.000000 | 5.000000 | PASS | assertion passed |
| ecu_latched_fault | 39 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| ecu_latched_fault | 40 | FAULT | ECU | ECU CLEAR | RECOVERY latched=0 | PASS | ECU fault injected |
| ecu_latched_fault | 41 | ASSERT | ecu_state | EQ 6.000000 | 6.000000 | PASS | assertion passed |
| ecu_latched_fault | 42 | ASSERT | ecu_fault_latched | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| ecu_latched_fault | 43 | END |  |  |  | PASS | test case ended |
