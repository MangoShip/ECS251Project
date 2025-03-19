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
    Extract the BFS traversal time from the line containing "traversal time:" and return the average time.
    """
    times = []
    # This regex will match lines like:
    # "Serial BFS traversal time: 123456789 ns" or "Parallel (OpenMP) BFS traversal time: 123456789 ns"
    pattern = re.compile(r"traversal time:\s*([\d.]+) ns")

    for _ in range(num_runs):
        proc = subprocess.run([executable] + arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        output = proc.stdout
        match = pattern.search(output)
        if match:
            time_val = float(match.group(1))
            times.append(time_val)
        else:
            print(f"Warning: traversal time not found in output from {executable} with arguments {arguments}")

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
    Plot the performance data. X-axis: node count, Y-axis: time in seconds.
    Each program will have its own line marker.
    """
    programs = sorted(set(row['program_name'] for row in data))
    fig, ax = plt.subplots()

    for prog in programs:
        x = [row['node_count'] for row in data if row['program_name'] == prog]
        y = [row['time'] / 1e9 for row in data if row['program_name'] == prog]
        ax.plot(x, y, marker='o', label=prog)

    ax.set_xlabel("Node Count")
    ax.set_ylabel("Time (seconds)")
    ax.set_xscale("linear")
    ax.set_yscale("linear")
    ax.legend()
    ax.set_title(title_str)
    plt.savefig(output_image)


def create_pivot_table(data, output_csv):
    """
    Create a pivot table from the performance data and write it to a CSV file.
    """
    programs = sorted(set(row['program_name'] for row in data))
    sizes = sorted(set(row['node_count'] for row in data))
    headers = ["node_count"] + programs
    table = []

    for s in sizes:
        row_dict = {}
        for prog in programs:
            entry = next((e for e in data if e['node_count'] == s and e['program_name'] == prog), None)
            if entry is not None:
                row_dict[prog] = f"{entry['time'] / 1e9:.6f}"
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
    fig, ax = plt.subplots(figsize=(len(df.columns) * 2, len(df) * 0.5 + 1))
    ax.axis('tight')
    ax.axis('off')
    the_table = ax.table(cellText=df.values, colLabels=df.columns, loc='center', cellLoc='center')
    plt.savefig(image_file, bbox_inches='tight')


def main():
    # Predefined node sizes
    node_sizes = [10, 100, 1000, 2000, 5000, 10000, 50000, 100000, 500000, 1000000, 10000000]
    thread_count = 8
    num_runs = 5

    # Define the four BFS executables and their parameters.
    # For serial, only the node count is passed.
    programs = [
        {
            "name": "serial_BFS",
            "executable": "./target/serial_BFS",
            "args": []  # only node count appended
        },
        {
            "name": "openmp_BFS",
            "executable": "./target/openmp_BFS",
            "uses_threads": True
        },
        {
            "name": "pthread_BFS",
            "executable": "./target/parallel_BFS",
            "uses_threads": True
        },
        {
            "name": "tholder_BFS",
            "executable": "./target/tholder_BFS",
            "uses_threads": True
        }
    ]

    fieldnames = ["node_count", "program_name", "num_threads", "time"]
    collected_data = []

    for size in node_sizes:
        for prog in programs:
            if prog.get("uses_threads", False):
                # For parallel programs, pass node_count and thread count as arguments
                run_args = [str(size), str(thread_count)]
                avg_time = run_executable(prog["executable"], run_args, num_runs=num_runs)
                if avg_time is None:
                    print(f"Skipping {prog['name']} for node count {size} due to missing timing data.")
                    continue
                collected_data.append({
                    "node_count": size,
                    "program_name": prog["name"],
                    "num_threads": thread_count,
                    "time": avg_time
                })
                print(
                    f"Collected: node_count={size}, program={prog['name']}, threads={thread_count}, time={avg_time} ns")
            else:
                # For serial version, pass only the node count.
                run_args = [str(size)]
                avg_time = run_executable(prog["executable"], run_args, num_runs=num_runs)
                if avg_time is None:
                    print(f"Skipping {prog['name']} for node count {size} due to missing timing data.")
                    continue
                collected_data.append({
                    "node_count": size,
                    "program_name": prog["name"],
                    "num_threads": 1,
                    "time": avg_time
                })
                print(f"Collected: node_count={size}, program={prog['name']}, time={avg_time} ns")

    # Append collected data to CSV
    output_csv = "performance_data.csv"
    append_to_csv(output_csv, collected_data, fieldnames)
    print(f"Performance data appended to {output_csv}")

    # Generate a plot of results and pivot table image
    output_image = "performance_plot.png"
    pivot_csv = "pivot_table.csv"
    pivot_image = "pivot_table.png"

    plot_results(collected_data, output_image, "BFS Performance Comparison")
    create_pivot_table(collected_data, pivot_csv)
    generate_table_image(pivot_csv, pivot_image)

    print(f"Generated graph image: {output_image}")
    print(f"Generated pivot table CSV: {pivot_csv}")
    print(f"Generated pivot table image: {pivot_image}")


if __name__ == "__main__":
    main()
