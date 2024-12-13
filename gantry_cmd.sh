#!/bin/bash

# Set the workspace and cluster variables
WORKSPACE="ronanlb-workspace-internal"
CLUSTER="ai2/mosaic-cirrascale"
BUDGET="ai2/oe-eval"

# Run the gantry command
gantry run --beaker-image 'ai2/pytorch1.11.0-cuda11.3-python3.10' --workspace "$WORKSPACE" --cluster "$CLUSTER" --budget "$BUDGET" -- python run.py 
