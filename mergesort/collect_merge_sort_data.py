#!/usr/bin/env python3
import argparse
import subprocess
import csv
import re
import os
import matplotlib.pyplot as plt

def run_executable(executable, arguments, num_runs=5):
    """
    Run the given executable with arguments num_runs times.
    Extract the merge sort time from the line starting with "PERFDATA" and return the average time.
    """
    times = []
    pattern = re.compile(r'^PERFDATA,(\d+),([^,]+),(\d+),(\d+),(\d+),([\d.]+)', re.MULTILINE)
    for _ in range(num_runs):
        proc = subprocess.run([executable] + arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        output = proc.stdout
        match = pattern.search(output)
        if match:
            time_val = float(match.group(6))
            times.append(time_val)
        else:
            print(f"Warning: PERFDATA line not found in output from {executable} with arguments {arguments}")
    if times:
        return sum(times) / len(times)
    else:
        return None

def append_to_csv(csv_file, data, fieldnames):
    """
    Append the given data (list of dictionaries) to the CSV file.
    If the file does not exist or is empty, write the header first.
    """
    file_exists = os.path.isfile(csv_file)
    write_header = not file_exists or os.path.getsize(csv_file) == 0
    with open(csv_file, 'a', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()
        for row in data:
            writer.writerow(row)

def plot_results(data, output_image, title_str):
    """
    Plot the performance data. X-axis: array size, Y-axis: time in nanoseconds.
    Different colors/markers for each program.
    """
    programs = sorted(set(row['program_name'] for row in data))
    plt.figure()
    for prog in programs:
        x = [row['array_size'] for row in data if row['program_name'] == prog]
        y = [row['time'] for row in data if row['program_name'] == prog]
        plt.plot(x, y, marker='o', label=prog)
    plt.xlabel("Array Size")
    plt.ylabel("Time (nanoseconds)")
    plt.xscale("log")
    plt.yscale("log")
    plt.legend()
    plt.title(title_str)
    plt.savefig(output_image)

def main():
    parser = argparse.ArgumentParser(description="Data collection pipeline for merge sort performance.")
    parser.add_argument("array_sizes", nargs="+", type=int, help="List of array sizes (for instance, 100 1000 100000 ...)")
    parser.add_argument("--threads", type=int, default=4, help="Number of threads for parallel versions")
    parser.add_argument("--min_parallel_size", type=int, default=10, help="Minimum subarray size to use for parallel threads")
    parser.add_argument("--thread_stack_size", type=int, default=(1 << 20), help="Thread stack size for each thread (in bytes)")
    parser.add_argument("--runs", type=int, default=5, help="Number of runs for each configuration (to average)")
    parser.add_argument("--output_csv", type=str, default="performance_data.csv", help="Output CSV's file name")
    parser.add_argument("--output_image_name", type=str, default="performance_plot.csv", help="File name for the graph png image")
    parser.add_argument("--title_str", type=str, default="Performance Comparison", help="Title for the graph of results")
    args = parser.parse_args()

    # Define the programs with their executable names and how to pass arguments.
    # For serialMergeSort, no parallel parameters are used.
    programs = [
        {
            "name": "serialMergeSort",
            "executable": "./serialMergeSort",
            "args": []  # Only the array size will be appended.
        },
        {
            "name": "parallelMergeSort",
            "executable": "./parallelMergeSort",
            # The new flags: <num_threads> then optional flags -m and -s, then the array size.
            "args": [str(args.threads), "-m", str(args.min_parallel_size), "-s", str(args.thread_stack_size)]
        },
        {
            "name": "openmpMergeSort",
            "executable": "./openmpMergeSort",
            # Similarly: <num_threads> then optional flags -m and -s, then the array size.
            "args": [str(args.threads), "-m", str(args.min_parallel_size), "-s", str(args.thread_stack_size)]
        }
    ]

    fieldnames = ["array_size", "program_name", "num_threads", "min_parallel_size", "thread_stack_size", "time"]
    collected_data = []

    for size in args.array_sizes:
        for prog in programs:
            # For serial version, just append the size; for the others, append the already‐defined arguments plus the size.
            run_args = prog["args"] + [str(size)]
            avg_time = run_executable(prog["executable"], run_args, num_runs=args.runs)
            if avg_time is None:
                print(f"Skipping {prog['name']} for array size {size} due to missing timing data.")
                continue
            # For the serial version, record fixed parameters.
            if prog["name"] == "serialMergeSort":
                num_threads = 1
                min_parallel = 0
                thread_stack = 0
            else:
                num_threads = args.threads
                min_parallel = args.min_parallel_size
                thread_stack = args.thread_stack_size
            collected_data.append({
                "array_size": size,
                "program_name": prog["name"],
                "num_threads": num_threads,
                "min_parallel_size": min_parallel,
                "thread_stack_size": thread_stack,
                "time": avg_time
            })
            print(f"Collected: size={size}, program={prog['name']}, time={avg_time} ns")

    append_to_csv(args.output_csv, collected_data, fieldnames)
    print(f"Performance data appended to {args.output_csv}")

    plot_results(collected_data, args.output_image_name, args.title_str)

if __name__ == "__main__":
    main()
