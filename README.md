# SignalDesk — Newsroom Crisis and Narrative Intelligence System

C17 menu-driven coursework for Project 35. The newsroom functions **measure textual overlap and verification priority**, never truth or falsehood. No external libraries required.

## Start (macOS / Linux)

```sh
make
./build/signaldesk
```

For a quick demonstration:

```sh
./build/signaldesk --scenario 1   # normal newsroom
./build/signaldesk --scenario 2   # influx and limited capacity
./build/signaldesk --scenario 3   # sorting and search stress
./build/signaldesk --workbench-all # all required syllabus algorithms
make test
make check-rules
make sanitize
sh scripts/demo.sh
```

Run commands from the repository root to locate `data/`. The executable is `build/signaldesk`, not `./signaldesk`.

**Reduced day-to-day workflow:** use menu choices 1 (dashboard), 2 (articles), 3 (text overlap), 4 (event tracking), 5 (assignments), 9 (three scenarios), and 0 (exit). Algorithm implementations remain because every assessed algorithm in SPEC Appendix B is mandatory. Menu 7 exposes all 36 only when needed for viva; no unnecessary second implementation has been introduced.

## Repository contents

- `src/` and `include/`: independent, testable algorithm library plus newsroom CLI.
- `data/`: 48 synthetic articles, 8 sources, 6 events and 5 fact-checkers.
- `tests/`: unit and integration tests with seeded random cases.
- `docs/USER_GUIDE.md`: menu and command walkthrough.
- `docs/DESIGN_GUIDE.md`: architecture, data ownership and engineering decisions.
- `docs/DEMONSTRATION.md` and `scripts/demo.sh`: assessment demo.
- `reports/complexity_report.md`: complexity reference for all 36 entries.

The benchmark menu currently invokes the reproducible stress comparison, not the complete seven-trial CSV benchmarking specified in SPEC §7. No benchmark values are hardcoded.

## Release tagging (after committing on your computer)

```sh
git add .
git commit -m "SignalDesk integrated project and documentation"
git tag -a v0.9.0-rc1 -m "SignalDesk coursework release candidate"
git push origin main --tags
```

The tag is **not** created merely by unzipping this archive.
