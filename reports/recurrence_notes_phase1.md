# SignalDesk Phase 1: Recurrences and complexity

All complexities count dominant comparisons or multiplications, not measured runtime.
The Master Theorem applies to balanced recurrences of the form T(n)=aT(n/b)+f(n)
with constant a>=1 and b>1 (up to usual rounding).

| Algorithm | Recurrence | Master Theorem | Time | Extra space |
|---|---|---|---|---|
| Merge sort | T(n)=2T(n/2)+Theta(n) | Case 2: f(n)=Theta(n^log_2 2 log^0 n) | Theta(n log n), best/average/worst | Theta(n) buffer and Theta(log n) stack |
| Binary search (either form) | T(n)=T(n/2)+Theta(1) | Case 2: n^log_2 1=1 | Theta(log n) worst; Theta(1) best | iterative Theta(1), recursive Theta(log n) |
| Quick sort average (balanced in expectation) | T(n) approximately 2T(n/2)+Theta(n) | Case 2 for balanced approximation | Theta(n log n) average | O(log n) stack due to smaller-side recursion |
| Quick sort worst | T(n)=T(n-1)+Theta(n) | Not applicable: partitions do not shrink by a fixed factor | Theta(n^2) | O(log n) stack due to smaller-side recursion |
| Fast power | T(e)=T(e/2)+Theta(1) | Case 2: n^log_2 1=1 | Theta(log e) for e>0; Theta(1) for e=0 | Theta(1) |
| Strassen | T(n)=7T(n/2)+Theta(n^2) | Case 1: n^log_2 7 dominates n^2 | Theta(n^log_2 7), about n^2.807 | O(n^2) scratch and padding |

## Notes

- Merge sort is stable; this quick sort uses a deterministic median-of-three pivot
  and a three-way partition to handle equal keys efficiently. Median-of-three
  does not prevent quadratic worst-case patterns.
- Binary search returns the first occurrence using the lower-bound invariant:
  every position below `lo` is smaller than the target, and the first possible
  match is in `[lo, hi)`.
- The recursive Strassen implementation uses a classical multiplication cutoff
  of 32; input dimensions are padded to the next power of two and outputs trimmed.
  For matrices smaller than the cutoff, runtime is cubic.
- For integer exponentiation, multiplication may return an overflow status.
  Modular exponentiation uses repeated doubling to avoid overflow during
  modular products. All `ops` counters can be omitted by passing NULL.
- The quick-sort *average* claim refers to expected partition behavior over
  input distributions, not an expectation over randomized pivots: this pivot
  selection is fully deterministic.
