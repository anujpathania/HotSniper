import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import re
import os.path
from pathlib import Path

# Aggregated subcomponent data into core level data.
# Subcomponent values can be combined using mean (default) or max.

# Return list of core names in the dataframe df.
def get_core_names(df):
    return ['C_{}'.format(i) for i in range(get_num_cores(df))]

# Return the number of cores in the dataframe header.
# Core numbering starts at 0.
def get_num_cores(df):
    core_list = df.columns.tolist()
    core_numbers = [int(re.search(r'C_(\d+)', core).group(1)) for core in core_list if re.search(r'C_(\d+)', core)]
    max_core = max(core_numbers)
    return max_core + 1

# Process df according to core_level flag.
# If core_level is true aggregate subcomp values into core values.
# If core_level is false split the 'df' into multiple data frames, one for
# each core.
def get_core_aggregate(df, core_level=False, atype='sum'):
    cores = get_core_names(df)

    # Core level, return list with one df with aggregated values.
    if core_level:
        core_df = pd.DataFrame()
        for core in cores:
            if atype == 'max':
                core_df[core] = df.filter(regex='{}_*'.format(core)).max(axis=1)
            elif atype == 'min':
                core_df[core] = df.filter(regex='{}_*'.format(core)).min(axis=1)
            else:
                core_df[core] = df.filter(regex='{}_*'.format(core)).sum(axis=1)

        return [("-Cores", core_df)]

    # Subcomponents, return list with df for every core with subcomp data.
    dfs = []
    for core in cores:
        dfs.append(("-" + core, df.filter(regex='{}_*'.format(core))))
    return dfs

# Read data from file, aggregate subcomponents if needed and plot values.
def plot_periodic_log(filename, core_level=False, no_display=True,
        atype='sum', x_label='Time (ms)', y_label='Metric'):
    df_all = pd.read_csv(filename, delim_whitespace=True)
    dfs = get_core_aggregate(df_all, core_level, atype)

    filename, _ = os.path.splitext(filename) # remove .gz, confuses suffix
    for (plot_name, df) in dfs:
        do_plot(df, filename, plot_name, no_display, x_label, y_label)

# Plot dataframe 'fd' to display and write plot to file in pdf, png and eps.
def do_plot(df, filename, plot_name='', no_display=True,
        x_label='Time (ms)', y_label='Metric'):
    fig, axs = plt.subplots(figsize=(12,12))
    df.plot(ax=axs)
    fp = Path(filename)
    axs.set_xlabel(x_label)
    axs.set_ylabel(y_label)
    axs.set_title(fp.stem + plot_name)

    # Create plot files in multiple formats.
    # for suf in ['.pdf', '.png', '.eps']: # TODO: make this an option
    for suf in ['.png']:
        fp_suf = fp.with_name(fp.stem + plot_name)
        fp_suf  = fp_suf.with_suffix(suf)
        fig.savefig(str(fp_suf))

    if no_display == False:
        plt.show()
    plt.close(fig)

if __name__ == "__main__":
    argparser = argparse.ArgumentParser(description="Generic plotting")
    argparser.add_argument("filename", help="data file")
    argparser.add_argument("--no-display", default=False,
            action='store_true',
            help="Do not open a plot window")
    argparser.add_argument("--core-level", default=False,
            action='store_true', help="Plot at core level")
    argparser.add_argument("--type", default='sum',
            help="Aggregate can be 'sum', 'max' or 'min'")
    args = argparser.parse_args()

    plot_periodic_log(args.filename, args.core_level, args.no_display, args.type)
