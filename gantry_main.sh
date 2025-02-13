#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="aai2/jupiter-cirrascale-2"
BUDGET="ai2/oe-eval"

# Run the gantry command
gantry run --gpus 8 --workspace "$WORKSPACE" --cluster "$CLUSTER" --budget "$BUDGET" -- python main.py
