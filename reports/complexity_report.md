# Algorithm complexity report (36 workbench entries)

All bounds are asymptotic unless explicitly qualified. `n` denotes input length, `V` vertices, `E` edges, `m` pattern length, `k` symbols or item count, and `C` integer capacity. These are theoretical bounds, **not measured benchmark values**.

| Group | Algorithm | Time complexity | Auxiliary space | Edge cases |
|---|---|---|---|---|
| Sorting and searching | Merge Sort | O(n log n) | O(n) | empty, duplicate and ordered data |
| Sorting and searching | Quick Sort | O(n log n) average, O(n^2) worst | O(log n) average stack | empty, duplicates and reverse order |
| Sorting and searching | Iterative Binary Search | O(log n) | O(1) | empty and duplicate keys |
| Sorting and searching | Recursive Binary Search | O(log n) | O(log n) | empty and missing target |
| Divide and conquer | Maximum Subarray | O(n log n) | O(log n) | negative-only and ties |
| Divide and conquer | Fast Exponentiation | O(log exponent) | O(1) | zero exponent and overflow |
| Divide and conquer | Celebrity Identification | O(n) | O(1) | none or single source |
| Divide and conquer | Classical Matrix Multiplication | O(n^3) | O(1) extra | zero-size and one-size |
| Divide and conquer | Strassen Matrix Multiplication | O(n^log2(7)) | O(n^2) workspace per recursion level | odd sizes and cutoff |
| Greedy | Activity Selection | O(n log n) | O(n) | touching and empty intervals |
| Greedy | Interval Partitioning | O(n log n) | O(n) | touching boundaries |
| Greedy | Fractional Knapsack | O(n log n) | O(n) | zero capacity or weight |
| Greedy | Huffman Coding | O(n + k log k) | O(k + encoded bits) | empty and one symbol |
| Greedy | Dijkstra | O((V+E) log V) | O(V+E) | unreachable and negative edges |
| Greedy | Prim | O(V^2) | O(V) | disconnected network |
| Greedy | Kruskal | O(E log E) | O(V+E) | equal weights and disconnected |
| Dynamic programming | 0/1 Knapsack DP | O(n*capacity) | O(n*capacity) | zero capacity and overweight |
| Dynamic programming | Matrix Chain Order | O(n^3) | O(n^2) | zero or one matrix |
| Dynamic programming | Longest Common Subsequence | O(a*b) | O(a*b) | empty and disjoint tokens |
| Dynamic programming | Optimal Binary Search Tree | O(n^3) | O(n^2) | zero frequencies and empty |
| Dynamic programming | Bellman-Ford | O(V*E) | O(V) | negative cycle and unreachable |
| Dynamic programming | Floyd-Warshall | O(V^3) | O(V^2) | unreachable and negative cycles |
| Dynamic programming | Travelling Salesperson DP | O(n^2*2^n) | O(n*2^n) | one city and cap of 15 |
| Backtracking and branch and bound | N-Queens | O(n!) worst | O(n) | unsatisfiable n=2,3 |
| Backtracking and branch and bound | Sum of Subsets | O(2^n) worst | O(n + output) | zero target and unreachable |
| Backtracking and branch and bound | 0/1 Knapsack Backtracking | O(2^n) worst | O(n) | empty and overweight |
| Backtracking and branch and bound | FIFO Branch and Bound | O(2^n) worst | O(2^n) frontier | empty and zero capacity |
| Backtracking and branch and bound | LIFO Branch and Bound | O(2^n) worst | O(2^n) frontier | equal-density items |
| Backtracking and branch and bound | Least-Cost Branch and Bound | O(2^n log 2^n) worst | O(2^n) frontier | zero-value items |
| Flow and matching | Ford-Fulkerson | O(E*|maxflow|) integral | O(V^2) | zero capacities and disconnected |
| Flow and matching | Maximum Bipartite Matching | O(V*E) | O(V+E) | no compatible pairs |
| Number theory | Chinese Remainder Theorem | O(k log M) | O(1) extra | noncoprime and overflow |
| String matching | Naive Matching | O(n*m) | O(1) | overlap and empty pattern |
| String matching | Rabin-Karp | O(n+m) expected; O(n*m) worst | O(1) | collisions and overlaps |
| String matching | Finite-Automaton Matching | O(n + 256*m) | O(256*m) | empty and repeated patterns |
| String matching | KMP | O(n+m) | O(m) | repeated and overlapping patterns |

## Recurrence methods

**Merge Sort.** `T(n)=2T(n/2)+Theta(n)`. Master Theorem case 2 with `a=2,b=2,f(n)=Theta(n)`: `Theta(n log n)`.

**Binary Search.** `T(n)=T(n/2)+Theta(1)`. Master Theorem case 2 with `a=1,b=2,f(n)=Theta(1)`: `Theta(log n)` for nonempty input.

**Quick Sort.** Balanced/expected partitions give `T(n)=2T(n/2)+Theta(n)`, case 2, `Theta(n log n)`; worst-case partition gives `T(n)=T(n-1)+Theta(n)`, `Theta(n^2)`, to which the standard Master Theorem does not apply.

**Fast Exponentiation.** `T(e)=T(e/2)+Theta(1)`, case 2, `Theta(log e)`.

**Strassen.** `T(n)=7T(n/2)+Theta(n^2)`, Master Theorem case 1, `Theta(n^log2(7))`, approximately `Theta(n^2.807)`. Padding and cutoff affect constants.

Other techniques include substitution to prove bounds, expanding recurrences iteratively, and recursion trees that sum per-level work. A valid Master Theorem application requires appropriate subproblem ratios and asymptotic regularity; e.g., unbalanced quicksort does not fit its usual form.

## Correctness invariants

Sorting: each finalized merge/partition preserves the multiset of inputs. Search: binary-search intervals preserve every possible first match. Greedy interval allocation: desks hold no overlapping intervals. Dynamic programming: each table cell holds an optimum for its exact subproblem. Shortest-path relaxation never creates a path shorter than the optimal valid path. Branch and bound: nodes pruned by an admissible fractional bound cannot improve the incumbent. Matching: each augmenting path increases matching cardinality by one. Huffman: emitted prefix-free codes can be decoded exactly.

## Complexity classes

Decision versions of polynomially solvable problems (sorting checks, shortest path, bipartite matching) are in P. Subset Sum and 0/1 Knapsack decision problems are NP-complete under binary encoding; their optimization versions are NP-hard. TSP decision (tour length at most B) is NP-complete; optimization TSP is NP-hard. N-Queens existence is a distinct question from enumerating all solutions, which may require exponential output size. Avoid calling an optimization problem itself NP-complete.

**Polynomial reduction: Subset Sum to decision Knapsack.** For nonnegative integers `a1...an` and target `T`, create a knapsack item `i` with weight `wi=ai` and value `vi=ai`, set weight capacity `W=T`, and target value `V=T`. A subset meeting `sum ai=T` fits `sum wi<=T` and achieves `sum vi>=T`. Conversely, because weights equal values, both inequalities force equality `sum ai=T`. The mapping copies `n` numbers and is polynomial time.

## Empirical measurement

Run `./build/signaldesk --scenario 3` to produce measured comparison output on the local machine. Do not cite numerical timings without capturing the corresponding run. The complete seven-trial median and CSV benchmark engine remains a follow-up task.
