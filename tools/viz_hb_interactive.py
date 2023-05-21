import argparse
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np


def get_column_data(filepath, col_number):
    col_data = []
    with open(filepath, "r") as f:
        next(f)  # Skip column header line
        for line in f:
            columns = line.split()
            col_data.append(columns[col_number])

    return col_data


def get_interval_diffs(data):
    if not isinstance(data, list):
        raise TypeError("data must be a list of integers")

    diffs = []
    prev_element = None

    for cur_element in data:
        if not isinstance(cur_element, int):
            raise TypeError("found cur_element of list that is not of integer type")

        if prev_element is None:
            prev_element = cur_element
            continue

        diffs.append(cur_element - prev_element)
        prev_element = cur_element

    return diffs


def plot(data, grid_density=100):
    plt.figure(figsize=(10, 3))
    plt.xscale("linear")
    plt.autoscale(True, "x", True)

    plt.xlabel("Simulation time (ns)")
    plt.ylabel("Value")
    plt.xticks(rotation=45, ha="right")
    plt.yticks([0, 1])

    plt.vlines(data, ymin=0, ymax=1, linewidth=1)

    ax = plt.gca()  # get current axis
    ax.xaxis.set_major_locator(MaxNLocator(grid_density))
    ax.ticklabel_format(useOffset=False, style="plain")

    plt.show()


def plot_interval_diff(data, grid_density=None):
    interval_diffs = get_interval_diffs(data)

    # Freedman-Diaconis rule
    q1, q3 = np.percentile(interval_diffs, [25, 75])
    iqr = q3 - q1
    n = len(interval_diffs)
    bin_size = 2 * iqr / (n ** (1 / 3))
    bin_count = int(np.ceil((max(interval_diffs) - min(interval_diffs)) / bin_size))
    bin_count = bin_count if bin_count < n else n

    # Plot it!
    plt.figure(figsize=(10, 5))
    plt.hist(interval_diffs, bins=bin_count, color="blue", edgecolor="black")
    plt.title("Interval difference histogram")
    plt.xlabel("Interval difference (ns)")
    plt.ylabel("Frequency")
    plt.xticks(rotation=45, ha="right")

    ax = plt.gca()
    ax.xaxis.set_major_locator(MaxNLocator(grid_density if grid_density else bin_count))
    ax.ticklabel_format(useOffset=False, style="plain")

    plt.show()


def main():
    plot_types = ["impulse", "interval-difference"]

    parser = argparse.ArgumentParser(
        description="Interactively show heartbeat data from specified file"
    )
    parser.add_argument(
        "filepath",
        metavar="filepath",
        type=str,
        help="path to file containing heartbeat data",
    )
    parser.add_argument(
        "--grid-density",
        metavar="density",
        type=int,
        default=100,
        help="density of grid ticks, useful for getting higher/lower interval labels",
    )
    parser.add_argument(
        "--plot-type",
        metavar="type",
        type=str,
        choices=plot_types,
        default="impulse",
        help=f"type of plot to visualize, types are: {', '.join(plot_types)}",
    )
    args = parser.parse_args()

    timestamps = get_column_data(args.filepath, 2)
    timestamps = [int(x) for x in timestamps]

    if args.plot_type == "impulse":
        plot(timestamps, args.grid_density)
    elif args.plot_type == "interval-difference":
        plot_interval_diff(timestamps, args.grid_density)
    else:
        print("provided plot type not supported")
        exit(1)


if __name__ == "__main__":
    main()
