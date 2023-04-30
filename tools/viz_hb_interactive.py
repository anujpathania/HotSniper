import argparse
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator


def get_column_data(filepath, col_number):
    col_data = []
    with open(filepath, "r") as f:
        next(f)  # Skip column header line
        for line in f:
            columns = line.split()
            col_data.append(columns[col_number])

    return col_data


def plot(data, grid_density=100):
    plt.figure(figsize=(60, 10))
    plt.xscale("linear")
    plt.autoscale(True, "x", True)

    plt.xlabel("Simulation time (ns)")
    plt.ylabel("Value")
    plt.xticks(rotation=45, ha="right")

    plt.vlines(data, ymin=0, ymax=1, linewidth=1)

    plt.grid(True, linestyle="--", color="gray", linewidth=0.5, alpha=0.5, which="both")

    ax = plt.gca()  # get current axis
    ax.xaxis.set_major_locator(MaxNLocator(grid_density))
    ax.ticklabel_format(useOffset=False, style="plain")

    plt.show()


def main():
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
        metavar="grid-density",
        type=int,
        default=100,
        help="density of grid ticks, useful for getting higher/lower interval labels",
    )
    args = parser.parse_args()

    timestamps = get_column_data(args.filepath, 2)
    timestamps = [int(x) for x in timestamps]

    plot(timestamps, args.grid_density)


if __name__ == "__main__":
    main()
