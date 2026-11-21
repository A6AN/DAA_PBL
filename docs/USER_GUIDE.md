# User guide

## Build

Install Apple Clang (Xcode Command Line Tools) or GCC. From the repository root run `make`; then `./build/signaldesk`. No account or network access is needed.

## Menu

| Choice | Action |
|---|---|
| 1 | Show sample newsroom statistics and a representative workflow |
| 2 | Load articles; list by time, sort by urgency, or search a preconfigured sample phrase |
| 3 | List potential textual-overlap article pairs and matching passages |
| 4 | Enter event ID 1–6 to show its timeline and coverage surge |
| 5 | Run before/after fact-checker capacity scenario |
| 6 | Show sample graph algorithm runs |
| 7 | Choose one of 36 algorithm demonstrations |
| 8 | Run reproducible stress comparison |
| 9 | Choose one of the three scenarios |
| 10 | Write `signaldesk_articles_export.csv` |
| 0 | Exit |

Invalid numeric choices are rejected and EOF ends the menu cleanly. Text-search phrase in option 2 is currently a demonstration constant, not an arbitrary typed query.

## Presentation route (short)

1. `./build/signaldesk --scenario 1` — dashboard, overlapping narratives, timelines and assignments.
2. `./build/signaldesk --scenario 2` — simulate incoming stories and reduced assignment capacity.
3. `./build/signaldesk --scenario 3` — repeatable stress comparison with actual machine timings.
4. `./build/signaldesk` → 7 → algorithm number — show the actual algorithm, not a canned result.
5. `make test` — show correctness evidence.

The data is synthetic. Similarity detects textual overlap, **not** factual accuracy. Benchmark timings vary by device.

## Troubleshooting

- `No such file or directory` for data: run from the project root.
- `make: command not found`: install developer tools.
- Sanitizer reports: save full log and investigate before release.
- `Operation failed`: the CLI prints a negative status code; consult `include/common.h`.
