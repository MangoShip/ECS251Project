#!/usr/bin/env python3
import argparse
import subprocess
import csv
import re
import os
import matplotlib.pyplot as plt
import pandas as pd

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
    Plot the performance data. X-axis: array size, Y-axis: time in seconds.
    Different colors/markers for each program.
    """
    programs = sorted(set(row['program_name'] for row in data))
    fig, ax = plt.subplots()

    for prog in programs:
        x = [row['array_size'] for row in data if row['program_name'] == prog]
        y = [row['time']/1e9 for row in data if row['program_name'] == prog]
        ax.plot(x, y, marker='o', label=prog)

    ax.set_xlabel("Array Size")
    ax.set_ylabel("Time (seconds)")
    ax.set_xscale("linear")
    ax.set_yscale("linear")

    ax.xaxis.set_major_formatter(plt.FuncFormatter(lambda x, pos: f"{int(x)}"))
    ax.yaxis.set_major_formatter(plt.FuncFormatter(lambda y, pos: f"{y:g}"))

    ax.legend()
    ax.set_title(title_str)

    plt.savefig(output_image)

def create_pivot_table(data, output_csv):
    """
    Table for the performance data.
    """
    programs = sorted(set(row['program_name'] for row in data))
    sizes = sorted(set(row['array_size'] for row in data))
    headers = ["size"] + programs
    table = []

    for s in sizes:
        row_dict = {}

        for prog in programs:
            entry = next((e for e in data if e['array_size'] == s and e['program_name'] == prog), None)
            
            if entry is not None:
                row_dict[prog] = f"{entry['time']/1e9:.6f}"
            else:
                row_dict[prog] = ""

        table.append([s] + [row_dict[prog] for prog in programs])

    with open(output_csv, 'w', newline='') as f:
        writer = csv.writer(f, delimiter='\t')
        writer.writerow(headers)

        for row in table:
            writer.writerow(row)

def generate_table_image(csv_file, image_file):
    df = pd.read_csv(csv_file, delimiter='\t')
    fig, ax = plt.subplots(figsize=(len(df.columns)*2, len(df)*0.5+1))

    ax.axis('tight')
    ax.axis('off')

    the_table = ax.table(cellText=df.values, colLabels=df.columns, loc='center', cellLoc='center')
    plt.savefig(image_file, bbox_inches='tight')

def main():
    parser = argparse.ArgumentParser(description="Data collection pipeline for merge sort performance.")
    parser.add_argument("array_sizes", nargs="+", type=int, help="List of array sizes (for instance, 100 1000 100000 ...)")
    parser.add_argument("--threads", nargs="+", type=int, default=[4], help="List of thread counts for parallel versions")
    parser.add_argument("--min_parallel_size", type=int, default=10, help="Minimum subarray size to use for parallel threads")
    parser.add_argument("--thread_stack_size", type=int, default=(1 << 20), help="Thread stack size for each thread (in bytes)")
    parser.add_argument("--runs", type=int, default=5, help="Number of runs for each configuration (to average)")
    parser.add_argument("--output_csv", type=str, default="performance_data.csv", help="Output CSV's file name")
    parser.add_argument("--output_image_name", type=str, default="performance_plot.png", help="File name for the graph png image")
    parser.add_argument("--title_str", type=str, default="Performance Comparison", help="Title for the graph of results")
    parser.add_argument("--pivot_csv", type=str, default="pivot_table.csv", help="Output pivot table CSV file name")
    parser.add_argument("--pivot_image", type=str, default="pivot_table.png", help="Output pivot table image file name")
    
    args = parser.parse_args()

    # For serialMergeSort, no parallel parameters are used.
    programs = [
        {
            "name": "serialMergeSort",
            "executable": "./target/serialMergeSort",
            "args": []  # Only the array size will be appended.
        },
        {
            "name": "parallelMergeSort",
            "executable": "./target/parallelMergeSort",
            "uses_threads": True
        },
        {
            "name": "openmpMergeSort",
            "executable": "./target/openmpMergeSort",
            "uses_threads": True
        },
        {
            "name": "openmpMergeSortIterative",
            "executable": "./target/openmpMergeSortIterative",
            "uses_threads": True
        },
        {
            "name": "parallelMergeSortIterative",
            "executable": "./target/parallelMergeSortIterative",
            "uses_threads": True
        },
        {
            "name": "tholderMergeSortIterative",
            "executable": "./target/tholderMergeSortIterative",
            "uses_threads": True
        },
        {
            "name": "serialMergeSortIterative",
            "executable": "./target/serialMergeSortIterative",
            "args": [] # Only the array size will be appended.
        }
    ]

    fieldnames = ["array_size", "program_name", "num_threads", "min_parallel_size", "thread_stack_size", "time"]
    collected_data = []

    for size in args.array_sizes:
        for prog in programs:
            # If the program uses threads, iterate over the provided thread counts.
            if prog.get("uses_threads", False):
                for thread_count in args.threads:
                    run_args = [str(thread_count), "-m", str(args.min_parallel_size), "-s", str(args.thread_stack_size), str(size)]
                    avg_time = run_executable(prog["executable"], run_args, num_runs=args.runs)

                    if avg_time is None:
                        print(f"Skipping {prog['name']} for array size {size} and threads {thread_count} due to missing timing data.")
                        continue

                    collected_data.append({
                        "array_size": size,
                        "program_name": prog["name"],
                        "num_threads": thread_count,
                        "min_parallel_size": args.min_parallel_size,
                        "thread_stack_size": args.thread_stack_size,
                        "time": avg_time
                    })

                    print(f"Collected: size={size}, program={prog['name']}, threads={thread_count}, time={avg_time} ns")
            else:
                # For serial versions, just append the size.
                run_args = prog.get("args", []) + [str(size)]
                avg_time = run_executable(prog["executable"], run_args, num_runs=args.runs)

                if avg_time is None:
                    print(f"Skipping {prog['name']} for array size {size} due to missing timing data.")
                    continue

                collected_data.append({
                    "array_size": size,
                    "program_name": prog["name"],
                    "num_threads": 1,
                    "min_parallel_size": 0,
                    "thread_stack_size": 0,
                    "time": avg_time
                })

                print(f"Collected: size={size}, program={prog['name']}, time={avg_time} ns")

    append_to_csv(args.output_csv, collected_data, fieldnames)
    print(f"Performance data appended to {args.output_csv}")

    # Separate plot and table files for each unique thread count.
    # For serial versions, num_threads is recorded as 1.
    unique_threads = sorted({row["num_threads"] for row in collected_data})
    for t in unique_threads:
        # Create file names by appending _<thread_count> before the extension.
        base_graph, ext_graph = os.path.splitext(args.output_image_name)
        graph_filename = f"{base_graph}_{t}{ext_graph}"
        
        base_pivot_csv, ext_pivot_csv = os.path.splitext(args.pivot_csv)
        pivot_csv_filename = f"{base_pivot_csv}_{t}{ext_pivot_csv}"
        
        base_pivot_img, ext_pivot_img = os.path.splitext(args.pivot_image)
        pivot_img_filename = f"{base_pivot_img}_{t}{ext_pivot_img}"
        
        # Filter the data for the current thread count.
        filtered_data = [row for row in collected_data if row["num_threads"] == t]
        
        plot_results(filtered_data, graph_filename, args.title_str)
        create_pivot_table(filtered_data, pivot_csv_filename)
        generate_table_image(pivot_csv_filename, pivot_img_filename)
        
        print(f"Generated graph: {graph_filename}")
        print(f"Generated pivot table CSV: {pivot_csv_filename}")
        print(f"Generated pivot table image: {pivot_img_filename}")

if __name__ == "__main__":
    main()
