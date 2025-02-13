import math
import itertools


def var_id(i, j, n):
    """
    Map the pair (i, j), with i < j, to a 1-based variable index.
    A simple scheme: var_id(i,j,n) = (i*(n-1) - i*(i+1)//2) + (j - i)
    or anything that is injective for 1 <= i < j <= n.
    """
    # Ensure i < j
    if i > j:
        i, j = j, i
    # One simple scheme (not the only one):
    # Number of variables in row i is (n - i).
    # We'll do a triangular indexing:
    return (i - 1) * (n) - (i - 1) * i // 2 + (j - i)


def write_dimacs_no_ramsey(n=800, r=10, s=10, outfile="ramsey.cnf"):
    """
    Generate a SAT instance requiring that the graph on n vertices
    has no clique of size t and no independent set of size t.

    *WARNING*:
      This direct approach (for t=10, n=800) is astronomically large
      and is not practically solvable with naive methods.
    """

    # Total number of variables = number of pairs i<j
    # That is n*(n-1)/2
    num_vars = n * (n - 1) // 2

    # We will build clauses in memory just to illustrate,
    # but for large n, one would typically write them directly to a file
    # (streaming) to avoid huge memory usage.
    clauses = []

    # --- Encode "no K_t" (no clique of size t) ---
    # For every t-subset of vertices, add a clause
    #    (¬x_{u1,u2} ∨ ¬x_{u1,u3} ∨ ... ) over all pairs in the t-subset.
    # This single clause says "it's not possible for all edges
    # among these t vertices to be True simultaneously."
    for subset in itertools.combinations(range(1, n + 1), r):
        # All pairwise edges among this subset:
        edges = list(itertools.combinations(subset, 2))
        # We want a disjunction of negated variables:
        # i.e. at least one must be false.
        clause = []
        for (a, b) in edges:
            clause.append(-var_id(a, b, n))  # negative literal -> ¬x_{a,b}
        clauses.append(clause)

    # --- Encode "no I_t" (no independent set of size t) ---
    # For every t-subset of vertices, add a clause
    #   ( x_{u1,u2} ∨ x_{u1,u3} ∨ ... )
    # meaning "not all edges among these t vertices can be false."
    for subset in itertools.combinations(range(1, n + 1), s):
        edges = list(itertools.combinations(subset, 2))
        clause = []
        for (a, b) in edges:
            clause.append(var_id(a, b, n))  # positive literal -> x_{a,b}
        clauses.append(clause)

    # Now we write this out in DIMACS CNF format
    with open(outfile, "w") as f:
        # number of clauses:
        num_clauses = len(clauses)

        f.write(f"p cnf {num_vars} {num_clauses}\n")
        for c in clauses:
            line = " ".join(str(lit) for lit in c) + " 0\n"
            f.write(line)

    print(f"CNF with {num_vars} variables and {len(clauses)} clauses written to {outfile}.")


if __name__ == "__main__":
    n = 17
    r = 4
    s = 4
    write_dimacs_no_ramsey(n, r, s, outfile="ramsey_" + str(n) + "_" + str(r) + "_" + str(s) + " .cnf")
    # Then you would call, e.g.:
    #   plingeling ramsey.cnf > ramsey.out
    # But in practice, this is too large to handle directly!