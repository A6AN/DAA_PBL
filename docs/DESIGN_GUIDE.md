# Design guide

## Objective

Model an offline digital newsroom with article ingestion, textual-overlap discovery, event volume analysis, and limited fact-checker allocation. The product does not determine whether stories are true.

## Architecture

- **Models** (`include/models.h`): articles, sources, events, fact-checkers and tasks.
- **Shared utilities** (`include/common.h`): consistent errors, checked allocation, optional operation counters and seeded LCG.
- **Algorithm library** (`src/sorting_searching/`, `src/divide_conquer/`, `src/greedy/`, `src/dynamic_programming/`, `src/backtracking/`, `src/graphs_flow_matching/`, `src/string_matching/`, `src/number_theory/`): independent assessed implementations.
- **Product** (`src/newsroom/article_manager.c`, `newsroom.c`): stores and imports articles, calls existing algorithms without rewriting them.
- **Presentation** (`src/main.c`, `workbench.c`, `scenarios.c`): a CLI, a single workbench metadata table and three seeded demos.
- **Tests** (`tests/test_*.c`): independent executables with reference oracles.

## Ownership

ArticleStore grows with checked realloc; caller frees through `article_store_free`. Graph constructors return caller-owned graph objects with paired free functions. Array/list outputs allocated by newsroom algorithms are caller-owned unless a header says otherwise. All CSV files are caller-owned FILE streams; the reader never closes them.

## Data format

`data/articles.csv`, `sources.csv`, `events.csv`, and `fact_checkers.csv` contain synthetic records. CSV supports quoted fields, commas, doubled quotes and multiline fields. Bad records are reported with record-line context when recoverable.

## Correctness and reproducibility

Seeded LCG tests cover sorting, shortest paths, LCS, allocation, matching and numerical algorithms. Benchmarks and demos are deterministic in input selection, but wall-clock timings depend on hardware. Every assessed algorithm is retained to satisfy Project 35, although the ordinary demo uses a small subset.

## Current limitations

- Some CLI choices call a demonstration scenario rather than a persistent, editable in-memory newsroom session.
- Assignment tasks are indivisible work units; generalized multi-hour scheduling is not implemented.
- Menu 8 provides a small stress comparison, not the full seven-trial median CSV benchmark lab.
- Source-reference networks are synthetic graph examples.
