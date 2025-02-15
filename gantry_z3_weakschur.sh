#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="ai2/saturn-cirrascale"
BUDGET="ai2/oe-eval"

# Run the gantry command
for n in 585 590 600 610 620 630 640 646 647 648 649 650
do
  gantry run --name "ws_$n" --preemptible --gpus 1 --workspace "$WORKSPACE" --cluster "$CLUSTER" --budget "$BUDGET" -- python z3_weakschur.py -n "$n" -k 6
done