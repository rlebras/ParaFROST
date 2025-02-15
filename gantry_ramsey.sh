#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="ai2/saturn-cirrascale"
BUDGET="ai2/oe-eval"

# Run the gantry command
for n in 102
do
  gantry run --name "r_$n" --preemptible --gpus 1 --workspace "$WORKSPACE" --cluster "$CLUSTER" --budget "$BUDGET" -- python ramsey.py -n "$n" -s 6 -t 6
done