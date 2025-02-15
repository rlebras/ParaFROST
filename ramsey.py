import torch
import argparse
import itertools
import random
import numpy

def sample_subsets(n, subset_size, num_samples):
    """
    Randomly sample 'num_samples' subsets of size 'subset_size'
    from the set {0, 1, ..., n-1}.
    Returns a list of tuples.
    """
    subsets = []
    for _ in range(num_samples):
        subset = tuple(random.sample(range(n), subset_size))
        subsets.append(subset)
    return subsets


def approximate_clique_score(A, subsets):
    """
    Given a continuous adjacency matrix A in [0, 1],
    and a list of subsets (each a tuple of vertices),
    compute the average 'product of edges' over those subsets.

    The product of edges in subset S is:
        product_{i<j in S} A[i,j]
    """
    # We'll accumulate the products for each subset, then average.
    # A is a torch.Tensor on GPU or CPU, shape (n, n)
    # subsets is a list of tuples
    products = []
    for S in subsets:
        # For each subset, collect edges A[i, j] for i<j in S
        # We can do this in a vectorized way:
        # Indices for i<j in S:
        # One approach is to gather all pairs i<j from S
        # Then multiply them together
        edges = []
        for i in range(len(S)):
            for j in range(i + 1, len(S)):
                edges.append(A[S[i], S[j]])
        # product of edges in S
        product_S = torch.prod(torch.stack(edges))
        products.append(product_S)
    # Average over subsets
    return torch.mean(torch.stack(products))


def approximate_independent_set_score(A, subsets):
    """
    Similar to 'approximate_clique_score', but for independent sets.
    We want the product of (1 - A[i,j]) for i<j in S.
    """
    products = []
    for S in subsets:
        edges = []
        for i in range(len(S)):
            for j in range(i + 1, len(S)):
                # Probability that i,j is NOT an edge is (1 - A[i,j])
                edges.append(1 - A[S[i], S[j]])
        product_S = torch.prod(torch.stack(edges))
        products.append(product_S)
    return torch.mean(torch.stack(products))


def clamp_and_symmetrize(P):
    """
    Enforce symmetry and no self-loops on parameter matrix P.
    For example, we can just mirror the upper triangle
    and set diagonal to a large negative value (forcing sigmoid -> ~0).
    """
    # Mirror the upper triangle
    P = torch.triu(P, diagonal=1) + torch.triu(P, diagonal=1).T
    # Set diagonal to a large negative => A[ii] ~ 0 after sigmoid
    diag_indices = torch.arange(P.shape[0], device=P.device)
    P[diag_indices, diag_indices] = -10.0
    return P


def run_gradient_search(n, s, t,
                        s_subsets_per_iter=50,
                        t_subsets_per_iter=50,
                        lr=0.1,
                        iters=1000):
    """
    Attempt a gradient-based approach to minimize the approximate
    count (probability) of having an s-clique or a t-independent set.

    - n: number of vertices
    - s, t: clique size and independent-set size
    - s_subsets_per_iter, t_subsets_per_iter: how many random subsets to sample each iteration
    - lr: learning rate
    - iters: number of gradient steps

    Returns the final adjacency (rounded) as a [n,n] matrix of 0/1.
    """
    device = 'cuda' if torch.cuda.is_available() else 'cpu'

    # Initialize raw parameters P in (-0.1, 0.1), shape (n, n)
    # We'll keep them on GPU if available.
    P = 0.1 * (2 * torch.rand((n, n), device=device) - 1)
    P.requires_grad = True

    # Simple optimizer
    optimizer = torch.optim.Adam([P], lr=lr)

    for step in range(iters):
        optimizer.zero_grad()

        # Enforce symmetry / no self loops on raw param
        P.data = clamp_and_symmetrize(P.data)

        # Build adjacency in [0,1] via sigmoid
        A = torch.sigmoid(P)

        # Sample random subsets to approximate clique and independent-set presence
        s_subsets = sample_subsets(n, s, s_subsets_per_iter)
        t_subsets = sample_subsets(n, t, t_subsets_per_iter)

        # approximate "probable" s-clique measure
        clique_score = approximate_clique_score(A, s_subsets)
        # approximate "probable" t-ind set measure
        ind_score = approximate_independent_set_score(A, t_subsets)

        # Combine into a single loss
        # (You might also weight them differently, e.g. clique_score + ind_score)
        loss = clique_score + ind_score

        # Backprop / gradient
        loss.backward()
        optimizer.step()

        if (step + 1) % 100 == 0:
            print(f"Iter {step + 1}/{iters} | Loss (approx) = {loss.item():.6f} | "
                  f"Clique_score={clique_score.item():.6f}, Ind_score={ind_score.item():.6f}")

    # Final adjacency
    with torch.no_grad():
        P = clamp_and_symmetrize(P)  # final clamp
        A = torch.sigmoid(P)

    # Round the adjacency to 0/1
    A_rounded = (A > 0.5).float()

    return A_rounded.cpu().numpy()


# Example usage
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Ramsey parameters")

    # Define arguments
    parser.add_argument("-n", type=int, required=True, help="Value for n")
    parser.add_argument("-s", type=int, required=True, help="Value for s")
    parser.add_argument("-t", type=int, required=True, help="Value for t")

    args = parser.parse_args()  # parse the command-line arguments

    n = args.n
    s = args.s
    t = args.t

    adj_matrix = run_gradient_search(n, s, t,
                                     s_subsets_per_iter=100,
                                     t_subsets_per_iter=100,
                                     lr=0.02,
                                     iters=10000)
    print("Final adjacency matrix (rounded):")
    print(adj_matrix)
