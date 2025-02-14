#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="ai2/jupiter-cirrascale-2"
BUDGET="ai2/oe-eval"

# Run the gantry command
gantry run --gpus 1 --workspace "$WORKSPACE" --cluster "$CLUSTER" --budget "$BUDGET" -- python glucose.py
