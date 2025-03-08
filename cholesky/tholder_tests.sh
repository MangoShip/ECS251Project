#!/bin/bash

sizes=(1000 2000 4000 8000)
threads=8
trials=10

for size in "${sizes[@]}"; do
    echo "Running $trials trials for matrix size $size with $threads threads..."
    for trial in $(seq 1 $trials); do
        ./target/cholesky_tholder_mod "$size" "$threads" "$trial"
    done
    echo "----------------------------------------------------------"
done