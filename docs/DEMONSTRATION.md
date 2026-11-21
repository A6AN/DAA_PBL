# Assessment demonstration (5–8 minutes)

Before starting: `make` and `make test`. Use project root as working directory.

1. **Product identity**: SignalDesk is a C17 offline newsroom simulator; it does not verify factual truth.
2. **Normal operation**: run `./build/signaldesk --scenario 1`; point out statistics, textual overlap and story timeline.
3. **Breaking-news shock**: run `./build/signaldesk --scenario 2`; compare assigned and unassigned work before and after extra stories.
4. **Algorithms as real code**: run `./build/signaldesk` → choice `7` → select `1` for Merge Sort; choose another entry to show complexity and edge cases.
5. **Comparisons**: run `./build/signaldesk --scenario 3`; explain fixed seed and that timings are measured, not invented.
6. **Test evidence**: run `make test` and `make check-rules`.

Talk track: textual overlap uses token LCS, which cannot infer meaning; matching allocates compatible tasks; shortest path and MSTs illustrate network cost; the assessed algorithm catalogue is separate from the streamlined daily workflow.

Do not claim full benchmark CSV export or multi-unit scheduling is finished; these are known limitations.
