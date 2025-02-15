#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="ai2/saturn-cirrascale"
BUDGET="ai2/oe-eval"

# Run the gantry command
gantry run --name ws450 --preemptible --gpus 1 --workspace "$WORKSPACE" --cluster "$CLUSTER" --budget "$BUDGET" -- python z3_weakschur.py
