#!/usr/bin/env python3
import argparse

from z3 import *

WS_1 = [[1, 2]]
WS_bis2 = [[1, 2, 4, 8], [3, 5, 6, 7]]
WS_2 = [[1, 2, 6], [3], [9]]

WS_SPLIT = [[1], [3], [7], [11]]

WS_3 = [[1, 2, 4, 8, 11, 22], [3, 5, 6, 7, 19, 21, 23], [9, 10, 12, 13, 14, 15, 18, 20]]
WS_4 = [[1, 2, 4, 8, 11, 22, 25],
        [3, 5, 6, 7, 19, 21, 23, 51, 52, 64, 65],
        [9, 10, 12, 13, 14, 15, 17, 18, 20, 54, 55, 61, 62],
        [24, 26, 27, 28, 29, 30, 33, 41, 42, 47, 49]]
WS_5 = [[1, 2, 4, 8, 11, 22, 25, 63, 69, 135, 140, 150, 155, 178, 183],
        [3, 5, 6, 7, 19, 21, 23, 51, 52, 64, 65, 66, 137, 138, 139, 151, 152, 153, 180, 181, 182, 194, 195],
        [9, 10, 12, 13, 14, 15, 17, 18, 20, 54, 55, 58, 59, 61, 62, 141, 142, 143, 144, 146, 147, 148, 149, 184, 185, 186, 187, 189, 191, 192],
        [24, 26, 27, 28, 29, 30, 33, 41, 42, 43, 44, 46, 47, 48, 49, 154, 156, 157, 158, 159, 160, 161, 162, 163, 170, 171, 172, 174, 175, 176, 177, 179],
        [67, 68, 70, 71, 72, 73, 74, 75, 85, 86, 87, 88, 114, 115, 118, 120, 127, 128, 129, 130, 131, 132, 133, 134, 136]]

WS_6 = [[1, 2, 4, 8, 11, 22, 25, 63, 69, 135, 140, 150, 155, 178, 183],
        [3, 5, 6, 7, 19, 21, 23, 51, 52, 64, 65, 66, 137, 138, 139, 151, 152, 153, 180, 181, 182, 194, 195],
        [9, 10, 12, 13, 14, 15, 17, 18, 20, 54, 55, 58, 59, 61, 62, 141, 142, 143, 144, 146, 147, 148, 149, 184, 185, 186, 187, 189, 191, 192],
        [24, 26, 27, 28, 29, 30, 33, 41, 42, 43, 44, 46, 47, 48, 49, 154, 156, 157, 158, 159, 160, 161, 162, 163, 170, 171, 172, 174, 175, 176, 177, 179],
        [67, 68, 70, 71, 72, 73, 74, 75, 85, 86, 87, 88, 114, 115, 118, 120, 127, 128, 129, 130, 131, 132, 133, 134, 136],
        [197, 198, 200, 203, 204, 205]]

WS_6_seq = [[1, 2, 4, 8, 11, 22, 25, 63, 69, 135, 140, 150, 155, 178, 183],
        [3, 5, 6, 7, 19, 21, 23, 51, 52, 64, 65, 66, 137, 138, 139, 151, 152, 153, 180, 181, 182, 194, 195],
        [9, 10, 12, 13, 14, 15, 17, 18, 20, 54, 55, 56, 57, 58, 59, 60, 61, 62, 141, 142, 143, 144, 145, 146, 147, 148, 149, 184, 185, 186, 187, 188, 189, 190, 191, 192],
        [24, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49, 154, 156, 157, 158, 159, 160, 161, 162, 163, 170, 171, 172, 174, 175, 176, 177, 179],
        [67, 68, 70, 71, 72, 73, 74, 75, 85, 86, 87, 88, 91, 114, 115, 118, 120, 127, 128, 129, 130, 131, 132, 133, 134, 136],
        [197, 198, 200, 203, 204, 205]]

WS_4_1 = [[1, 2, 4, 8, 11, 22, 25, 50, 53, 66], [3, 5, 6, 7, 19, 21, 23, 51, 64, 65], [9, 10, 12, 13, 14, 15, 16, 17, 18, 20, 54, 55, 56, 57, 58, 59, 60, 61, 62], [24, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49]]
WS_4_2 = [[1, 2, 4, 8, 11, 22, 25, 50, 53, 66], [3, 5, 6, 7, 19, 21, 23, 51, 64, 65], [9, 10, 12, 13, 14, 15, 16, 17, 18, 20, 54, 55, 56, 57, 58, 59, 60, 61, 62], [24, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49]]
WS_4_3 = [[1, 2, 4, 8, 11, 22, 25, 50, 53, 66], [3, 5, 6, 7, 19, 21, 23, 51, 64, 65], [9, 10, 12, 13, 14, 15, 16, 17, 18, 20, 54, 55, 56, 57, 58, 59, 60, 61, 62], [24, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49]]


WS_5_1 = [[1, 2, 4, 8, 11, 22, 25, 32, 53, 60, 63, 69, 76, 83, 89, 96, 119, 135, 140, 150, 155, 169, 178, 183, 196],
        [3, 5, 6, 7, 19, 21, 23, 35, 37, 50, 51, 52, 64, 65, 66, 77, 79, 109, 111, 113, 122, 123, 137, 138, 139, 151, 152, 153, 167, 180, 181, 182, 193, 194, 195],
        [9, 10, 12, 13, 14, 15, 16, 17, 18, 20, 54, 55, 56, 57, 58, 59, 61, 62, 141, 142, 143, 144, 145, 146, 147, 148, 149, 184, 185, 186, 187, 188, 189, 190, 191, 192],
        [24, 26, 27, 28, 29, 30, 31, 33, 34, 36, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 101, 103, 154, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 168, 170, 171, 172, 173, 174, 175, 176, 177, 179],
        [67, 68, 70, 71, 72, 73, 74, 75, 78, 80, 81, 82, 84, 85, 86, 87, 88, 90, 91, 92, 93, 94, 95, 97, 98, 99, 100, 102, 104, 105, 106, 107, 108, 110, 112, 114, 115, 116, 117, 118, 120, 121, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 136]]

WS_646 = [ [1, 2, 6, 10, 14],
           [3, 4, 5, 15, 16],
           [7, 8, 9],
           [11, 12, 13],
           [43, 44, 45],
           [96, 97, 98]
           ]

WS_646_Most = [
    [1, 2, 6, 10, 14, 18, 26, 30, 34, 42, 46, 50, 54, 62, 70, 79, 82, 90, 95, 99, 111],
    [3, 4, 5, 15, 16, 17, 27, 28, 29, 39, 40, 41, 47, 48, 49, 112, 113, 114, 120, 121],
    [7, 8, 9, 19, 20, 21, 22, 23, 24, 25, 35, 36, 37, 38, 87, 88, 89, 136, 137, 138, 150, 152, 153, 154],
    [11, 12, 13, 31, 32, 33, 51, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 124],
    [43, 44, 45, 52, 53, 55, 56, 57, 58, 59, 60, 61, 63, 64, 65, 66, 67, 68, 69, 71, 72, 73],
    [96, 97, 98, 116, 117, 118, 140, 141, 142, 160, 161, 162, 184, 185, 186, 204, 205, 206]
]

WS_646_More = [
    [1, 2, 6, 10, 14, 18, 26, 30, 34, 42, 46, 50, 54, 62, 70, 82, 90],
    [3, 4, 5, 15, 16, 17, 27, 28, 29, 39, 40, 41, 47, 48, 49, 112, 113, 114, 120, 121, 122],
    [7, 8, 9, 19, 20, 21, 22, 23, 24, 25, 35, 36, 37, 38, 87, 88, 89, 136, 137, 138, 150, 152, 153, 154],
    [11, 12, 13, 31, 32, 33, 51, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 124],
    [43, 44, 45, 52, 53, 55, 56, 57, 58, 59, 60, 61, 63, 64, 65, 66, 67, 68, 69, 71, 72, 73, 74, 75, 76, 77, 78, 80, 81, 83, 84, 85, 86],
    []
]

def check_if(v, l):
    row_index = None  # Will store the row in which v is found
    for i, row in enumerate(l):
        if v in row:
            row_index = i
            break
    return row_index


def main():
    parser = argparse.ArgumentParser(description="Weak Schur parameters")

    # Define arguments
    parser.add_argument("-n", type=int, required=True, help="Value for n")
    parser.add_argument("-k", type=int, required=True, help="Value for k")

    args = parser.parse_args()  # parse the command-line arguments

    n = args.n
    k = args.k

    x = [Int('x'+str(i+1)) for i in range(n)]

    # Create a solver instance
    set_param('parallel.enable', True)
    s = Solver()

    for i in range(n):
        s.add(x[i] >= 1, x[i] <= k)
    for j in range(n):
        for i in range(n):
            if i >= j:
                break
            if i+j+2 <= n:
                s.add(Implies(x[i] == x[j], x[i+j+1] != x[i]))

    rec = True
    if rec:
        for i in range(n):
            if i+1 < 3:
                s.add(x[i] < 2)
            elif i+1 < 9:
                s.add(x[i] < 3)
            elif i+1 < 24:
                s.add(x[i] < 4)
            elif i+1 < 67:
                s.add(x[i] < 5)
            elif i+1 < 197:
                s.add(x[i] < 6)

    ws646 = False
    if ws646:
        for i in range(n):
            if i+1 < 3:
                s.add(x[i] < 2)
            elif i+1 < 7:
                s.add(x[i] < 3)
            elif i+1 < 15:
                s.add(x[i] < 4)
            elif i+1 < 31:
                s.add(x[i] < 5)
            elif i+1 < 63:
                s.add(x[i] < 6)

    ws_2plus1 = False
    if ws_2plus1:
        for i in range(n):
            if i+1 < 3:
                s.add(x[i] < 2)
            elif i+1 < 7:
                s.add(x[i] < 3)
            elif i+1 < 11:
                s.add(x[i] < 4)
            elif i+1 < 1:
                s.add(x[i] < 5)
            elif i+1 < 1:
                s.add(x[i] < 6)

    ws_split = False
    if ws_split:
        for i in range(n):
            if i+1 < 3:
                s.add(x[i] < 2)
            elif i+1 < 7:
                s.add(x[i] < 3)
            elif i+1 < 11:
                s.add(x[i] < 4)
            elif i+1 < 43:
                s.add(x[i] < 5)
            elif i+1 < 96:
                s.add(x[i] < 6)


    for i in range(n):
        v = i+1
        row_index = check_if(v, WS_6)
        if row_index is not None:
            print("in it!", v, row_index)
            s.add(x[i] == row_index+1)


    # Check for satisfiability
    result = s.check()
    print("Solver result:", result)

    forced_assignment = []
    if result == sat:
        # If sat, we can get a model (one example of satisfying assignments)
        m = s.model()
        print("Model found:")
        l = [[] for j in range(k)]
        for i in range(n):
            v = int(str(m[x[i]]))-1
            l[v].append(i+1)
            forced_assignment.append(v+1)
        print(l)

        for i in range(n):
            s.push()
            s.add(x[i] != forced_assignment[i])
            res2 = s.check()
            if res2 == sat:
                forced_assignment[i] = -1
                print("Not forced: ", str(i + 1))
                s.pop()  # Revert to state before adding c2
            else:
                print("Forced: ", str(i+1), str(forced_assignment[i]))
                s.pop()  # Revert to state before adding c2
                s.add(x[i] == forced_assignment[i])

        # 2) Create a blocking clause to forbid this exact solution.
        #    We say: at least one variable in the model must differ
        #    from its value in the current model.
        more = False
        total_solutions = 1
        while more == True and total_solutions < 1000:
            block = []
            for d in m:  # m.decls() also works
                # d is a variable, and m[d] is the value assigned in this model
                block.append(d() != m[d])
            s.add(Or(block))

            # 3) Check again for satisfiability.
            if s.check() == sat:
                total_solutions += 1
                m = s.model()

                l = [[] for j in range(k)]
                for i in range(n):
                    v = int(str(m[x[i]])) - 1
                    l[v].append(i + 1)
                    if forced_assignment[i] != v+1:
                        forced_assignment[i] = -1

            else:
                more = False
                print("No other distinct solution.")
        print("Forced assignment:")
        l = [[] for j in range(k)]
        for i in range(n):
            if forced_assignment[i] == -1:
                continue
            v = forced_assignment[i] - 1
            l[v].append(i + 1)
        print(l)
    else:
        print("No solution found (unsat or unknown).")


if __name__ == "__main__":
    main()