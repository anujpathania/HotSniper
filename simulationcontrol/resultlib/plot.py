import os
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
SIMULATIONCONTROL = os.path.dirname(HERE)
sys.path.append(SIMULATIONCONTROL)

import matplotlib as mpl
mpl.use('Agg')
import math
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np
from resultlib import *
import seaborn as sns


def smoothen(data, k):
    return list(np.convolve(data, np.ones((k,))/k, mode='same'))


def interleave(list, k):
    stride = math.ceil(len(list) / k)
    for start in range(stride):
        for i in range(k):
            ix = start + stride * i
            if ix < len(list):
                yield list[ix]
        

def set_color_palette(num_colors):
    if num_colors > 0:
        pal = list(interleave(sns.color_palette("hls", num_colors), 1))
        sns.set_palette(pal)

def get_column_data(filepath, col_number, header_row):
    col_data = []
    with open(filepath, "r") as f:
        if header_row:
            next(f)  # Skip column header row

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


def plot_trace(run, name, title, ylabel, traces_function, active_cores, yMin=None, yMax=None, smooth=None, force_recreate=False):
    filename = os.path.join(find_run(run), '{}.png'.format(name))

    if not os.path.exists(filename) or force_recreate:
        try:
            traces = traces_function()
        except KeyboardInterrupt:
            raise
        except:
            print('>>> skipped {} {}'.format(run, name))
            return

        set_color_palette(len(active_cores))

        plt.figure(figsize=(14,10))
        tracelen = 0
        for core, trace in enumerate(traces):
            if core in active_cores:
                valid_trace = [value for value in trace if value is not None]
                if len(valid_trace) > 0:
                    #if yMin is not None:
                    #    yMin = min(yMin, min(valid_trace) * 1.1)
                    #if yMax is not None:
                    #    yMax = max(yMax, max(valid_trace) * 1.1)
                    tracelen = len(trace)
                    if smooth is not None:
                        trace = smoothen(trace, smooth)
                    plt.plot(trace, label='Core {}'.format(core))
        if yMin is not None:
            plt.ylim(bottom=yMin)
        if yMax is not None:
            plt.ylim(top=yMax)
    
        if name == 'rel_nuca_cpi' and 'x264-simsmall-1' in run:
            phases = [(0.01, 0.09, 'red'),
                      (0.12, 0.44, 'green'),
                      (0.46, 0.68, 'blue'),
                      (0.7, 0.78, 'orange'),
                      (0.8, 0.99, 'purple')]
        elif name == 'rel_nuca_cpi' and 'streamcluster-simsmall-3' in run:
            phases = [(0.1, 0.45, 'red'),
                      (0.65, 0.98, 'green')]
        else:
            phases = []
        for start, end, color in phases:
            start = int(start * tracelen)
            end = int(end * tracelen)
            plt.plot((start, start), (0.1, 0.4), c=color)
            plt.plot((end, end), (0.1, 0.4), c=color)
            plt.plot((start, end), (0.15, 0.15), c=color)

        plt.title('{} {}'.format(title, run))
        plt.legend()
        plt.xlabel('Time (ms)')
        plt.ylabel(ylabel)
        plt.grid()
        plt.grid(which='minor', linestyle=':')
        plt.minorticks_on()
        plt.savefig(filename, bbox_inches='tight')
        plt.close()


def plot_cpi_stack_trace(run, active_cores, force_recreate=False):
    blacklist = ['imbalance', 'sync', 'total', 'rs_full', 'serial', 'smt', 'mem-l4', 'mem-dram-cache', 'dvfs-transition', 'other']
    parts = [part for part in get_cpi_stack_trace_parts(run) if not any(b in part for b in blacklist)]
    traces = {part: get_cpi_stack_part_trace(run, part) for part in parts}

    set_color_palette(len(parts))

    for core in active_cores:
        name = 'cpi-stack-trace-c{}'.format(core)
        title = 'cpi-stack-trace Core {}'.format(core)
        filename = os.path.join(find_run(run), '{}.png'.format(name))
        if not os.path.exists(filename) or force_recreate:
            fig = plt.figure(figsize=(14,10))
            ax = fig.add_subplot(1, 1, 1)

            stacktrace = [traces[part][core] for part in parts]
            xs = range(len(stacktrace[0]))
            plt.stackplot(xs, stacktrace, labels=parts)
            plt.ylim(bottom=0, top=6)
            plt.title('{} {}'.format(title, run))

            handles, labels = ax.get_legend_handles_labels()
            ax.legend(handles[::-1], labels[::-1], loc='upper left', bbox_to_anchor=(1, 1))
            
            plt.xlabel('Time (ms)')
            plt.ylabel('CPI')
            plt.grid()
            plt.grid(which='minor', linestyle=':')
            plt.minorticks_on()
            plt.savefig(filename, bbox_inches='tight')
            plt.close()


def plot_hb_trace(run, force_recreate=False):
    final_results_path = find_run(run)

    pattern = r"^\d+\.hb.log$"
    for logfile in os.listdir(final_results_path):
        if not re.match(pattern, logfile):
            continue

        plot_file = os.path.join(final_results_path, '{}.png'.format(logfile.strip(".log")))
        if os.path.exists(plot_file) and not force_recreate:
            continue

        tags = get_column_data("%s/%s" % (final_results_path, logfile), 1, True)
        work_start_indicator = True if len(tags) > 0 and int(tags[0]) == -1 else False

        timestamps = get_column_data("%s/%s" % (final_results_path, logfile), 2, True) # Second col are timestamps
        timestamps = [int(x) for x in timestamps]

        plt.figure(figsize=(60,10))

        plt.xscale("linear")
        plt.autoscale(True, "x", True)

        plt.title('Heartbeats for app id {} - {}'.format(logfile.strip(".hb.log"), run))
        plt.xlabel("Simulation time (ns)")
        plt.ylabel("Value")
        plt.xticks(rotation=45, ha="right")
        plt.yticks([0, 1])

        if work_start_indicator:
            plt.vlines(timestamps[0], ymin=0, ymax=1, linewidth=1, colors="red")
            plt.vlines(timestamps[1:], ymin=0, ymax=1, linewidth=1, colors="blue")
        else:
            plt.vlines(timestamps, ymin=0, ymax=1, linewidth=1, colors="blue")

        ax = plt.gca()
        ax.xaxis.set_major_locator(MaxNLocator(200))
        ax.ticklabel_format(useOffset=False, style="scientific")

        plt.savefig(plot_file, bbox_inches="tight")
        plt.close()


def plot_hb_histogram(run, force_recreate=False):
    final_results_path = find_run(run)

    pattern = r"^\d+\.hb.log$"
    for logfile in os.listdir(final_results_path):
        if not re.match(pattern, logfile):
            continue

        plot_file = os.path.join(final_results_path, '{}.histogram.png'.format(logfile.strip(".log")))
        if os.path.exists(plot_file) and not force_recreate:
            continue

        tags = get_column_data("%s/%s" % (final_results_path, logfile), 1, True)
        work_start_indicator = True if len(tags) > 0 and int(tags[0]) == -1 else False

        timestamps = get_column_data("%s/%s" % (final_results_path, logfile), 2, True)
        timestamps = [int(x) for x in timestamps]
        interval_diffs = get_interval_diffs(timestamps)

        # Freedman-Diaconis rule
        q1, q3 = np.percentile(interval_diffs, [25, 75])
        iqr = q3 - q1
        n = len(interval_diffs)
        bin_size = 2 * iqr / (n ** (1 / 3))
        bin_count = int(np.ceil((max(interval_diffs) - min(interval_diffs)) / bin_size))
        bin_count = bin_count if bin_count < n else n

        plt.figure(figsize=(60,10))
<<<<<<< Updated upstream
        plt.hist(interval_diffs, bins=bin_count, color="blue", edgecolor="black")
=======

        plt.hist(interval_diffs[1:] if work_start_indicator else interval_diffs, bins=bin_count, color="blue", edgecolor="black")

>>>>>>> Stashed changes
        plt.title('Heartbeat interval difference histogram for app id {} - {}'.format(logfile.strip(".hb.log"), run))
        plt.xlabel("Values")
        plt.ylabel("Interval difference")
        plt.xticks(rotation=45, ha="right")

        ax = plt.gca()
        ax.xaxis.set_major_locator(MaxNLocator(bin_count))
        ax.ticklabel_format(useOffset=False, style="scientific")

        plt.savefig(plot_file, bbox_inches="tight")
        plt.close()


def create_plots(run, force_recreate=False):
    print('creating plots for {}'.format(run))
    active_cores = get_active_cores(run)

    plot_trace(run, 'frequency', 'Frequency', 'Frequency (GHz)', lambda: get_freq_traces(run), active_cores, yMin=0, yMax=4.1e9, force_recreate=force_recreate)
    plot_trace(run, 'temperature', 'Temperature', 'Temperature (C)', lambda: get_temperature_traces(run), active_cores, yMin=45, yMax=100, force_recreate=force_recreate)
    plot_trace(run, 'peak_temperature', 'Peak Temperature', 'Temperature (C)', lambda: get_peak_temperature_traces(run), [0], yMin=45, yMax=100, force_recreate=force_recreate)
    plot_trace(run, 'power', 'Power', 'Power (W)', lambda: get_power_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'utilization', 'Utilization', 'Utilization (%)', lambda: get_utilization_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'rel_nuca_cpi', 'Rel. NUCA CPI', 'Rel. NUCA CPI', lambda: get_rel_nuca_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'cpi', 'CPI', 'CPI', lambda: get_cpi_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    #plot_trace(run, 'abs_nuca_cpi', 'Abs. NUCA CPI', 'Abs. NUCA CPI', lambda: get_nuca_cpi_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'ips', 'IPS', 'IPS', lambda: get_ips_traces(run), active_cores, yMin=0, yMax=8e9, force_recreate=force_recreate)
    plot_trace(run, 'ipssmooth', 'Smoothed IPS', 'Smoothed IPS', lambda: get_ips_traces(run), active_cores, yMin=0, yMax=8e9, smooth=10, force_recreate=force_recreate)
    plot_cpi_stack_trace(run, active_cores, force_recreate=force_recreate)
    plot_hb_trace(run, force_recreate)
    plot_hb_histogram(run, force_recreate)

if __name__ == '__main__':
    for run in sorted(get_runs())[::-1]:
        create_plots(run)
