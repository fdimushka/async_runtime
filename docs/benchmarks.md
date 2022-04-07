## Benchmarks
Run on Intel(R) Core(TM) i9-9900K CPU @ 3.60GHz
CPU Caches:
- L1 Data 32 KiB (x8)
- L1 Instruction 32 KiB (x8)
- L2 Unified 256 KiB (x8)
- L3 Unified 16384 KiB (x1)
Load Average: 1.03, 0.98, 0.87

|Benchmark            |         Time       |      CPU  | Iterations|
| :---                |    :----:          | :----:    | :----:    |
|context switch       |       183 ns       |   183 ns  |    3770529|
|concurrent task_call |      3850 ns       | 0.004 ns  |    1000000|
|await empty task     |      6428 ns       |  4069 ns  |     173605|

