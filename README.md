# OpenMP_Mini1_Project

We built a small C++ benchmark to explore how OpenMP helps (or doesn't) for simple population-aggregation queries over a CSV dataset. The project reads CSV files in `data/PopulationData/`, loads them into a compact in-memory row-wise model, and measures several queries (sum/avg/max/min/top-K and per-country scans) in both serial and parallel modes.

This README is written by us and summarizes how to build, how to reproduce our runs, the measurements we collected on the real dataset and on a large synthetic dataset, and the concrete next steps we recommend.

## Quick build

We use CMake and target C++17. From the project root (macOS / zsh):

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j
```

Note: on macOS FindOpenMP sometimes fails with AppleClang. If that happens install Homebrew's `libomp` and re-run CMake:

```bash
brew install libomp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
``` 

Binaries we use below:
- `./build/OpenMP_Mini1_Project_app` — the main CSV-driven benchmark that reads a CSV and reports median/stddev timings.
- `./build/OpenMP_Mini1_Project_row_benchmark` — helper that generates a synthetic CSV and then invokes the app using that CSV (it writes `data/PopulationData/population_synth.csv`).

## How to run

Run the main app against the default CSV (relative path `data/PopulationData/population.csv`) and collect 5 repetitions (default):

```bash
./build/OpenMP_Mini1_Project_app -r 5 -t 4
```

Or point the app at any CSV using the `CSV_PATH` environment variable (we use this when driving the synthetic CSV):

```bash
CSV_PATH=data/PopulationData/population_synth.csv ./build/OpenMP_Mini1_Project_app -r 5 -t 4
```

The app prints per-query medians (microseconds) and sample standard deviations for serial and parallel runs, plus the measured result values so correctness can be spot-checked.

## Real dataset: run we executed

We ran the app against the included real CSV (`data/PopulationData/population.csv`) with 5 repetitions and 4 threads. Relevant output (exact lines):

```
Rows: 266 Years: 65
sumPopulationForYear: serial_t_median=0.875 us stddev=1.952, parallel_t_median=12.834 us stddev=622.549
  -> values: serial=57094730240 parallel=57094730240
averagePopulationForYear: serial_t_median=0.833 us stddev=0.146, parallel_t_median=11.542 us stddev=1.653
  -> values: serial=214641843.008 parallel=214641843.008
maxPopulationForYear: serial_t_median=1.125 us stddev=2.655, parallel_t_median=9.584 us stddev=1.795
  -> values: serial=5470271607 parallel=5470271607
minPopulationForYear: serial_t_median=0.917 us stddev=0.056, parallel_t_median=10.916 us stddev=1.841
  -> values: serial=0 parallel=0
topNCountriesByPopulationInYear: serial_t_median=12.000 us stddev=4.976, parallel_t_median=23.667 us stddev=6.738
  -> counts: serial_count=10 parallel_count=10
populationForCountryInYear: serial_t_median=22.750 us stddev=3.906, parallel_t_median=21.000 us stddev=0.427
  -> values: serial=70192 parallel=70192
poputationOverYearsForCountry: serial_t_median=21.750 us stddev=0.678, parallel_t_median=21.958 us stddev=0.494
  -> len=65
```

Interpretation: on the small real dataset absolute times are measured in microseconds and are tiny. For such small work items the fixed costs of parallelism (thread scheduling, reductions, heap work) are often larger than the actual per-iteration work; that makes the parallel timings noisier and sometimes larger than serial.

## Synthetic dataset: run we executed

We generated a synthetic CSV via `OpenMP_Mini1_Project_row_benchmark` (this writes `data/PopulationData/population_synth.csv` and then runs the app). We ran it for 200000 rows × 50 years, 5 repetitions and 4 threads. Key output (exact lines):

```
Rows: 200000 Years: 50
sumPopulationForYear: serial_t_median=2646.833 us stddev=443.159, parallel_t_median=949.750 us stddev=245.976
  -> values: serial=100393748336 parallel=100393748336
averagePopulationForYear: serial_t_median=2654.417 us stddev=302.089, parallel_t_median=950.625 us stddev=115.281
  -> values: serial=501968.742 parallel=501968.742
maxPopulationForYear: serial_t_median=2660.916 us stddev=211.314, parallel_t_median=1215.125 us stddev=156.254
  -> values: serial=999996 parallel=999996
minPopulationForYear: serial_t_median=2933.542 us stddev=196.876, parallel_t_median=1136.625 us stddev=120.713
  -> values: serial=13 parallel=13
topNCountriesByPopulationInYear: serial_t_median=15078.584 us stddev=358.045, parallel_t_median=1228.125 us stddev=28.640
  -> counts: serial_count=10 parallel_count=10
populationForCountryInYear: serial_t_median=36704.000 us stddev=690.066, parallel_t_median=36614.125 us stddev=1267.434
  -> values: serial=179310 parallel=179310
poputationOverYearsForCountry: serial_t_median=41210.750 us stddev=3135.146, parallel_t_median=37099.500 us stddev=773.019
  -> len=50
```

Interpretation: with a much larger dataset the parallel implementations clearly win for the year-centric scans (sum/avg/max/min) and for the top-K query. The top-K speedup is especially visible after we removed global synchronization and switched to per-thread min-heaps followed by a final merge.

We also note that per-country queries that return/allocate vectors (e.g., `poputationOverYearsForCountry`) are still expensive — they involve lookups and allocations and could be optimized by returning views or by reusing buffers when measuring.

## Concrete takeaways

- For year-centric aggregation (scan a single year across all countries) a columnar layout (one contiguous array per year) will be much faster and scale much better across threads than the current row-wise layout. The row-wise layout forces scattered reads when we scan per-year.
- Parallel overhead is visible on small inputs; use larger inputs to amortize it.
- Avoid global synchronization (we replaced a hot critical region in top-K with per-thread heaps and a merge step, which yielded large improvements on big inputs).
- Measure with multiple repetitions and use medians + sample stddev to reduce noise.

## Recommended next steps (we can implement these)

1. Add a columnar in-memory layout and a columnar CSV writer so we can run side-by-side comparisons (row-wise vs columnar) and quantify throughput differences.
2. Export benchmark results (CSV/JSON) and add a small script to plot speedups across thread counts and input sizes.
3. Optimize per-country APIs to return views or accept pre-allocated buffers to avoid allocation costs during measurement.

If you want us to proceed we can start with (1) and re-run the synthetic experiments to produce a performance report.

## Reproducible commands

Build and run the real dataset:

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -- -j
./build/OpenMP_Mini1_Project_app -r 5 -t 4
```

Generate and run the synthetic CSV (example 200k × 50):

```bash
rm -rf build && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . --config Release -- -j && cd .. && ./build/OpenMP_Mini1_Project_row_benchmark 200000 50 5 4
```

## Where to find the data

- Original CSV: `data/PopulationData/population.csv`
- Synthetic CSV (we create this when running the row benchmark): `data/PopulationData/population_synth.csv` (we intentionally do not overwrite the original CSV)

---

If you'd like, we'll implement the columnar layout next and add automated CSV export of benchmark runs so you can plot speedups across thread counts and dataset sizes.
