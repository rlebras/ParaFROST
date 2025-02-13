import torch
import json
import random


########################
# GPU-based operations #
########################

def generate_random_graph_gpu(num_vertices, edge_prob=0.5, seed=None):
    """
    Generate a random undirected graph on the GPU as an adjacency matrix.
    Each edge is included with probability edge_prob (Bernoulli).
    """
    if seed is not None:
        random.seed(seed)  # sets Python's random seed
        torch.manual_seed(seed)  # sets Torch seed (affects CUDA too)

    # Create a random matrix of shape (n, n) on GPU with uniform(0,1)
    # Then threshold it at edge_prob to decide edges (0/1).
    # We'll store it as an integer (0 or 1).
    A = (torch.rand((num_vertices, num_vertices), device='cuda') < edge_prob).float()

    # Symmetrize for an undirected graph
    # We'll do A = A OR A^T (logical or). But since it's 0/1, we can do max.
    A = torch.maximum(A, A.t())

    # Zero out the diagonal (no self-loops)
    idx = torch.arange(num_vertices, device='cuda')
    A[idx, idx] = 0

    # Return this GPU adjacency matrix
    return A


def has_triangle_gpu(A):
    """
    Return True if adjacency matrix A (on GPU) has at least one triangle.
    We use the formula: #triangles = trace(A^3) / 6.
    If that count > 0, there's a triangle.
    """
    # Matrix multiply on GPU: A^2, then (A^2)*A => A^3
    A2 = torch.matmul(A, A)
    A3 = torch.matmul(A2, A)

    # Number of triangles is trace(A3)/6 (for undirected, no loops).
    # If trace(A3) > 0, there's at least 1 triangle.
    trace_val = torch.trace(A3)
    return (trace_val.item() > 0)


##########################
# CPU-based operations   #
##########################

def has_independent_set_of_size_k_cpu(adj_matrix_2dlist, k=10):
    """
    Backtracking check for an independent set of size k.
    Here adj_matrix_2dlist is a standard Python 2D list of 0/1.
    We do this on the CPU because it's a combinatorial search
    that doesn't map easily to a simple GPU kernel.
    """
    n = len(adj_matrix_2dlist)

    def backtrack(chosen, start):
        if len(chosen) == k:
            return True
        # Prune if not enough vertices remain
        if n - start < k - len(chosen):
            return False

        for v in range(start, n):
            # Check if v is adjacent to any chosen vertex
            conflict = False
            for c in chosen:
                if adj_matrix_2dlist[c][v] == 1:
                    conflict = True
                    break
            if not conflict:
                chosen.append(v)
                if backtrack(chosen, v + 1):
                    return True
                chosen.pop()
        return False

    return backtrack([], 0)


def convert_torch_to_2dlist(A):
    """
    Convert a GPU (or CPU) torch Tensor (n x n) of 0/1 to a standard Python 2D list of ints.
    This is needed so our CPU backtracking can read adjacency easily.
    """
    A_cpu = A.to('cpu')  # move to CPU if on GPU
    # convert to nested list
    return A_cpu.tolist()


############################
# Main logic: random+check #
############################

def is_no_triangle_no_I10(G_gpu, k=10):
    """
    Check if the GPU adjacency matrix has NO triangle (K3)
    and NO independent set of size k=10.
    """
    # 1) Check for triangle on GPU
    if has_triangle_gpu(G_gpu):
        return False

    # 2) Check for independent set of size k on CPU
    #    Convert adjacency to 2D Python list first
    G_2dlist = convert_torch_to_2dlist(G_gpu)
    if has_independent_set_of_size_k_cpu(G_2dlist, k=k):
        return False

    return True


def find_graph_no_triangle_no_I10_gpu(
        num_vertices=40, edge_prob=0.45, max_tries=100, seed=None
):
    """
    Attempt up to max_tries random graphs on GPU.
    If we find one with no triangle and no independent set of size 10, return it (GPU matrix).
    Otherwise return None.
    """
    for attempt in range(max_tries):
        G_gpu = generate_random_graph_gpu(num_vertices, edge_prob=edge_prob, seed=(seed + attempt if seed else None))

        # Check conditions
        if is_no_triangle_no_I10(G_gpu, k=10):
            print(f"Success on attempt {attempt + 1}")
            return G_gpu
    print(f"No suitable graph found in {max_tries} attempts.")
    return None


def save_to_json_gpu(G_gpu, filename="graph_40_gpu.json"):
    """
    Save adjacency matrix (GPU tensor) to JSON in the requested format:
      {
        "Adjacency_matrix": [
           [0,1,0,...],
           [1,0,1,...],
           ...
        ]
      }
    """
    # Convert to a normal Python nested list
    G_list = convert_torch_to_2dlist(G_gpu)

    with open(filename, 'w') as f:
        json.dump({"Adjacency_matrix": G_list}, f, indent=2)
    print(f"Graph saved to {filename}")


########################
# Run the search logic #
########################

if __name__ == "__main__":
    n =30
    # Example usage:
    # We'll do up to 100 attempts, each time generating a random graph with p=0.45.
    # For 40 vertices, you might want to tweak p or increase max_tries if it's not found quickly.
    graph_gpu = find_graph_no_triangle_no_I10_gpu(
        num_vertices=n,
        edge_prob=0.45,
        max_tries=1000000,
        seed=42
    )

    if graph_gpu is not None:
        save_to_json_gpu(graph_gpu, filename="graph_" + str(n) + "_gpu.json")
    else:
        print("No success within the given attempts. Try increasing max_tries or adjusting edge_prob.")