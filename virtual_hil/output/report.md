# HIL Test Report

| Metric | Value |
|---|---:|
| Steps | 69 |
| Passed | 69 |
| Failed | 0 |
| Skipped | 0 |
| Errors | 0 |

| Case | Line | Step | Target | Expected | Actual | Status | Message |
|---|---:|---|---|---|---|---|---|
| power_up_normal | 2 | SET | throttle_position | 20 | 20 | PASS | signal set |
| power_up_normal | 3 | WAIT |  | 300 ms | 300 ms | PASS | wait completed |
| power_up_normal | 4 | ASSERT | engine_speed | GT 800.000000 | 2040.000000 | PASS | assertion passed |
| power_up_normal | 5 | ASSERT | battery_voltage | EQ 12.500000 | 12.500000 | PASS | assertion passed |
| power_up_normal | 6 | END |  |  |  | PASS | test case ended |
| set_signal | 9 | SET | coolant_temp | 55 | 55 | PASS | signal set |
| set_signal | 10 | ASSERT | coolant_temp | EQ 55.000000 | 55.000000 | PASS | assertion passed |
| set_signal | 11 | END |  |  |  | PASS | test case ended |
| wait_and_periodic_task | 14 | SET | throttle_position | 10 | 10 | PASS | signal set |
| wait_and_periodic_task | 15 | WAIT |  | 300 ms | 600 ms | PASS | wait completed |
| wait_and_periodic_task | 16 | ASSERT | engine_speed | GT 800.000000 | 1420.000000 | PASS | assertion passed |
| wait_and_periodic_task | 17 | ASSERT | engine_speed | LT 2000.000000 | 1420.000000 | PASS | assertion passed |
| wait_and_periodic_task | 18 | END |  |  |  | PASS | test case ended |
| fault_stuck | 21 | SET | throttle_position | 30 | 30 | PASS | signal set |
| fault_stuck | 22 | WAIT |  | 300 ms | 900 ms | PASS | wait completed |
| fault_stuck | 23 | FAULT | engine_speed | engine_speed STUCK 1500 | active from 900 ms | PASS | fault injected |
| fault_stuck | 24 | WAIT |  | 20 ms | 920 ms | PASS | wait completed |
| fault_stuck | 25 | ASSERT | engine_speed | EQ 1500.000000 | 1500.000000 | PASS | assertion passed |
| fault_stuck | 26 | WAIT |  | 120 ms | 1040 ms | PASS | wait completed |
| fault_stuck | 27 | ASSERT | engine_speed | GT 800.000000 | 2660.000000 | PASS | assertion passed |
| fault_stuck | 28 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| fault_stuck | 29 | END |  |  |  | PASS | test case ended |
| fault_offset | 32 | SET | throttle_position | 20 | 20 | PASS | signal set |
| fault_offset | 33 | WAIT |  | 300 ms | 1340 ms | PASS | wait completed |
| fault_offset | 34 | FAULT | engine_speed | engine_speed OFFSET 500 | active from 1340 ms | PASS | fault injected |
| fault_offset | 35 | WAIT |  | 20 ms | 1360 ms | PASS | wait completed |
| fault_offset | 36 | ASSERT | engine_speed | BETWEEN 2500.000000 2600.000000 | 2540.000000 | PASS | assertion passed |
| fault_offset | 37 | WAIT |  | 120 ms | 1480 ms | PASS | wait completed |
| fault_offset | 38 | ASSERT | engine_speed | GT 800.000000 | 2040.000000 | PASS | assertion passed |
| fault_offset | 39 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| fault_offset | 40 | END |  |  |  | PASS | test case ended |
| fault_open_circuit | 43 | SET | coolant_temp | 50 | 50 | PASS | signal set |
| fault_open_circuit | 44 | FAULT | coolant_temp | coolant_temp OPEN_CIRCUIT 0 | active from 1480 ms | PASS | fault injected |
| fault_open_circuit | 45 | WAIT |  | 20 ms | 1500 ms | PASS | wait completed |
| fault_open_circuit | 46 | ASSERT | coolant_temp | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| fault_open_circuit | 47 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| fault_open_circuit | 48 | END |  |  |  | PASS | test case ended |
| fault_dropout | 51 | SET | vehicle_speed | 80 | 80 | PASS | signal set |
| fault_dropout | 52 | FAULT | vehicle_speed | vehicle_speed DROPOUT 0 | active from 1500 ms | PASS | fault injected |
| fault_dropout | 53 | WAIT |  | 20 ms | 1520 ms | PASS | wait completed |
| fault_dropout | 54 | ASSERT | vehicle_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| fault_dropout | 55 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| fault_dropout | 56 | END |  |  |  | PASS | test case ended |
| reset_system | 59 | SET | battery_voltage | 15 | 15 | PASS | signal set |
| reset_system | 60 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| reset_system | 61 | ASSERT | battery_voltage | EQ 12.500000 | 12.500000 | PASS | assertion passed |
| reset_system | 62 | END |  |  |  | PASS | test case ended |
| signal_boundary_low | 65 | SET | engine_speed | 0 | 0 | PASS | signal set |
| signal_boundary_low | 66 | ASSERT | engine_speed | EQ 0.000000 | 0.000000 | PASS | assertion passed |
| signal_boundary_low | 67 | END |  |  |  | PASS | test case ended |
| signal_boundary_high | 70 | SET | engine_speed | 8000 | 8000 | PASS | signal set |
| signal_boundary_high | 71 | ASSERT | engine_speed | EQ 8000.000000 | 8000.000000 | PASS | assertion passed |
| signal_boundary_high | 72 | END |  |  |  | PASS | test case ended |
| repeated_fault_injection | 75 | SET | throttle_position | 25 | 25 | PASS | signal set |
| repeated_fault_injection | 76 | WAIT |  | 300 ms | 1820 ms | PASS | wait completed |
| repeated_fault_injection | 77 | FAULT | engine_speed | engine_speed STUCK 1200 | active from 1820 ms | PASS | fault injected |
| repeated_fault_injection | 78 | WAIT |  | 20 ms | 1840 ms | PASS | wait completed |
| repeated_fault_injection | 79 | ASSERT | engine_speed | EQ 1200.000000 | 1200.000000 | PASS | assertion passed |
| repeated_fault_injection | 80 | FAULT | engine_speed | engine_speed OFFSET 100 | active from 1840 ms | PASS | fault injected |
| repeated_fault_injection | 81 | WAIT |  | 20 ms | 1860 ms | PASS | wait completed |
| repeated_fault_injection | 82 | ASSERT | engine_speed | BETWEEN 1300.000000 2400.000000 | 1300.000000 | PASS | assertion passed |
| repeated_fault_injection | 83 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| repeated_fault_injection | 84 | END |  |  |  | PASS | test case ended |
| long_run_stability | 87 | SET | throttle_position | 15 | 15 | PASS | signal set |
| long_run_stability | 88 | WAIT |  | 1000 ms | 2860 ms | PASS | wait completed |
| long_run_stability | 89 | ASSERT | engine_speed | GT 800.000000 | 1730.000000 | PASS | assertion passed |
| long_run_stability | 90 | ASSERT | engine_speed | LT 3000.000000 | 1730.000000 | PASS | assertion passed |
| long_run_stability | 91 | RESET |  | reset ECU and faults | INIT | PASS | system reset |
| long_run_stability | 92 | END |  |  |  | PASS | test case ended |
