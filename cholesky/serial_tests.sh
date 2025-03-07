#!/bin/bash

sizes=(1000 2000 4000 8000)
trials=10

for size in "${sizes[@]}"; do
    echo "Running $trials trials for matrix size $size..."
    for trial in $(seq 1 $trials); do
        ./target/cholesky_serial "$size" "$trial"
    done
    echo "----------------------------------------------------------"
done