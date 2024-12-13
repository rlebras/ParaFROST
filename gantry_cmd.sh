#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="ai2/mosaic-cirrascale"
BUDGET="ai2/oe-eval"

# Run the gantry command
gantry run --workspace "$WORKSPACE" --cluster "$CLUSTER" -- python run.py --budget "$BUDGET"
