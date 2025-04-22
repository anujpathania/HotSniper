import datetime
import os
import shutil
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
SIMULATIONCONTROL = os.path.dirname(HERE)
BENCHMARKS = os.getenv("BENCHMARKS_ROOT")
RESULTS_DIR = os.path.join(os.getenv('GRAPHITE_ROOT'), 'results')
sys.path.append(SIMULATIONCONTROL)

import matplotlib as mpl
mpl.use('Agg')
import math
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator
import numpy as np
from resultlib import *
import seaborn as sns
import subprocess

try:
    import cv2
except Exception as e:
    print(e)

from resultlib import periodic_plot

# Returns the value of 'item' in the 'sim.cfg' file located in the
# results directory: results/'run'
# Note: we cannot import sniper_lib.py because it python2 code, so
# we call sniper_lib.py as an external program.
def get_config_val(run, item):
    cfg_file = os.path.join(os.getenv('GRAPHITE_ROOT'), 'results', run)
    sniper_config_prog = os.path.join(os.getenv('GRAPHITE_ROOT'), 'tools/sniper_lib.py')
    return subprocess.check_output(["python2", sniper_config_prog, cfg_file, item])

# Return the value of item in results/'run'/sim.cfg as a boolean.
def get_config_val_bool(run, item):
    return 'true' in str(get_config_val(run, item)).lower()


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


def plot_trace(run, name, title, ylabel, traces_function, active_cores, yMin=None, yMax=None, smooth=None, force_recreate=False, xlabel='Time (ms)'):
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
                    if yMin is not None:
                       yMin = min(yMin, min(valid_trace) * 1.1)
                    if yMax is not None:
                       yMax = max(yMax, max(valid_trace) * 1.1)
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
        plt.xlabel(xlabel)
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

        if(len(interval_diffs) < 2): continue

        int_diff_mean = np.mean(interval_diffs)

        # Freedman-Diaconis rule
        q1, q3 = np.percentile(interval_diffs, [25, 75])
        iqr = q3 - q1
        n = len(interval_diffs)
        bin_size = 2 * iqr / (n ** (1 / 3))

        if(bin_size == 0): return

        bin_count = int(np.ceil((max(interval_diffs) - min(interval_diffs)) / bin_size))
        bin_count = bin_count if bin_count < n else n

        plt.figure(figsize=(60,10))

        plt.hist(interval_diffs[1:] if work_start_indicator else interval_diffs, bins=bin_count, color="blue", edgecolor="black")

        plt.title('Heartbeat interval difference histogram for app id {} - {}'.format(logfile.strip(".hb.log"), run))
        plt.xlabel("Interval difference (ns)")
        plt.ylabel("Frequency")
        plt.xticks(rotation=45, ha="right")

        ax = plt.gca()
        ax.xaxis.set_major_locator(MaxNLocator(bin_count))
        ax.ticklabel_format(useOffset=False, style="scientific")
        ax.axvline(int_diff_mean, color='red', linestyle='dashed', linewidth=1, label='Mean')

        plt.savefig(plot_file, bbox_inches="tight")
        plt.close()


PIXEL_MAX = 255.0
def psnr(original, contrast):
    mse = np.mean((original - contrast) ** 2)

    if mse == 0:
        return 100
    
    psnr = 20 * math.log10(PIXEL_MAX/ math.sqrt(mse))
    return psnr


def parse_images(video_path: str) -> list:
    encoded_vid = cv2.VideoCapture(video_path)
            
    frames = []
    succ, img = encoded_vid.read()
    while succ:
        frames.append(img) 
        succ, img = encoded_vid.read()

    return frames


def get_benchmark_output(run, benchmark):
    run_out = []

    if benchmark in 'parsec-blackscholes':
        file =  open(os.path.join(run, 'output.txt'), 'r')
        for line in file.readlines():
            run_out += [float(num) for num in  re.findall(r'-?\b\d+\.?\d*\b', line)]

    elif benchmark in 'parsec-bodytrack':
        file =  open(os.path.join(run, 'poses.txt'), 'r')
        for line in file.readlines():
            run_out += [float(num) for num in  re.findall(r'-?\b\d+\.?\d*\b', line)]

    elif benchmark in'parsec-swaptions':
        execution_log =  io.TextIOWrapper(gzip.open(os.path.join(run, 'execution.log.gz'), 'r'), encoding="utf-8")
        for line in execution_log:
            m = re.search(r'SwaptionPrice: (\d+\.\d+) StdError: (\d+\.\d+)', line)
            if m is not None:
                run_out.append(float(m.group(1)))
                # run_out.append(float(m.group(2))) # std error printed with the options price

    elif benchmark in 'parsec-streamcluster':
        file =  open(os.path.join(run, 'output.txt'), 'r')
        for line in file.readlines():
            numbers = [float(num) for num in  re.findall(r'-?\b\d+\.?\d*\b', line)]

            if len(numbers) == 1:
                continue

            run_out += numbers

    elif benchmark in'parsec-canneal':
        execution_log =  io.TextIOWrapper(gzip.open(os.path.join(run, 'execution.log.gz'), 'r'), encoding="utf-8")
        
        for line in execution_log:
            m = re.search(r'Final routing is: (\d+\.\d+)', line)
            if m is not None:
                run_out += [float(m.group(1))]
                break

    elif benchmark in 'parsec-x264':        
        ref_file = os.path.join(SIMULATIONCONTROL, 'resultlib/resources', 'eledream_640x360_8.y4m') # TODO:only for simsmall.
        ref_images = parse_images(ref_file)

        run_file = os.path.join(run, 'eledream.264')
        run_images = parse_images(run_file)

        if(len(ref_images) != len(run_images)):
            print("Warning the reference video is different from the run video")
            return [0]*len(run_images)

        for ref_i, run_i in zip(ref_images, run_images):
            run_out.append(psnr(ref_i, run_i))
    
    return run_out


def calc_accuracy(run, ref, benchmark):
    comparison = list(zip(get_benchmark_output(run, benchmark), get_benchmark_output(ref, benchmark)))

    loss = 0
    for e in comparison:
        try:
            loss += abs(e[0] - e[1])/ abs(e[0])
        except:
            # print("warning zero div: trying calculation ({} - {} / {})".format(e[0], e[1], e[0]))
            continue
    
    try:
        return max(1 - (loss / len(comparison)), 0)
    except ZeroDivisionError:
        print("warning zero div: no output of the run")
        return 0


def prev_run_cleanup():
    pattern = r"^\d+\.hb.log$" # Heartbeat logs
    for f in os.listdir(BENCHMARKS):
        if not re.match(pattern, f):
            continue

        file_path = os.path.join(BENCHMARKS, f)
        if os.path.isfile(file_path):
            os.remove(file_path)
    
    # Benchmark Output
    for f in os.listdir(BENCHMARKS):
        if ('output.' in f) or ('.264' in f) or ('poses.' in f) or ('app_mapping' in f) :
            os.remove(os.path.join(BENCHMARKS, f))


def save_output_no_sim(benchmark, console_output, input_size, started, run):
    ref_run = 'results_no_sim_{}_{}_{}'.format(started, input_size, benchmark)
    directory = os.path.join(RESULTS_FOLDER, run, ref_run)
    
    if not os.path.exists(directory):
        os.makedirs(directory)

    with gzip.open(os.path.join(directory, 'execution.log.gz'), 'w') as f:
        f.write(console_output.encode('utf-8'))
    
    for f in os.listdir(BENCHMARKS):
        if 'output.' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)
        elif 'poses.' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)
        elif '.264' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)
        elif 'app_mapping.' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)

    return directory


def create_accuracy_reference(benchmark, input_size, run):
    config = [0 for _ in range(20)]
            
    prev_run_cleanup()

    cmd = os.path.join(os.getenv("BENCHMARKS_ROOT"), 'parsec/parsec-2.1/bin/parsecmgmt')
    
    proc_env = os.environ.copy()
    proc_env["MANUAL_PERFORATION"] = ','.join(map(str, config))

    console_output = ''
    p = subprocess.Popen([cmd, '-a', 'run', '-p', benchmark, '-i', input_size, '-n', '4', '-c',  'gcc-sniper'], 
                            env=proc_env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=1, cwd=BENCHMARKS)
    with p.stdout:
        for line in iter(p.stdout.readline, b''):
            linestr = line.decode('utf-8')
            console_output += linestr

    p.wait()

    dir = save_output_no_sim(benchmark, console_output, input_size, 
                        datetime.datetime.now(), run)

    return dir


app_map = { 0:'parsec-blackscholes',
            1:'parsec-bodytrack',
            2:'parsec-canneal',
            3:'parsec-streamcluster',
            4:'parsec-swaptions',
            5:'parsec-x264' }


def app_mapping(path):
    ids = []
    with open(path) as file:
        for line in file.readlines():
            ids.append(line.split(',')[1])

    return [app_map[int(id)] for id in ids]


def perforation_statistics(run):
    try:
        final_results_path = find_run(run)

        for benchmark in app_mapping(os.path.join(final_results_path, 'app_mapping.txt')):
            ref_results_path = create_accuracy_reference(benchmark, "simsmall", run)
            with open(os.path.join(RESULTS_DIR, run, 'accuracy-{}.txt'.format(benchmark)), 'w') as accuracy_file:
                accuracy_file.write("Final Accuracy: "+ str(calc_accuracy(final_results_path, ref_results_path, benchmark)))
    except Exception as e:
        print(e)


def create_plots(run, force_recreate=False):
    print('creating plots for {}'.format(run))
    active_cores = get_active_cores(run)

    # Create subcore and core level plots
    # NOTE: we assume a logging epoch of 1 ms, better to read
    # this from log file.

    # Thermal plots
    full_name = get_file(run, 'PeriodicThermal.log')
    periodic_plot.plot_periodic_log(full_name, core_level=False,
            y_label='Temperature (C)')
    periodic_plot.plot_periodic_log(full_name, core_level=True,
            atype='max', y_label='Temperature (C)' )

    # Power plots
    full_name = get_file(run, 'PeriodicPower.log')
    periodic_plot.plot_periodic_log(full_name, core_level=False,
            y_label='Power (W)')
    periodic_plot.plot_periodic_log(full_name, core_level=True,
            y_label='Power (W)')

    # R-value plots
    if get_config_val_bool(run, 'reliability/enabled'):
        print("-----")
        full_name = get_file(run, 'PeriodicRvalue.log')
        periodic_plot.plot_periodic_log(full_name, core_level=False,
                y_label='R-value')
        periodic_plot.plot_periodic_log(full_name, core_level=True,
                atype='min', y_label='R-value')

    plot_trace(run, 'frequency', 'Frequency', 'Frequency (GHz)', lambda: get_freq_traces(run), active_cores, yMin=0, yMax=4.1e9, force_recreate=force_recreate)
    plot_trace(run, 'peak_temperature', 'Peak Temperature', 'Temperature (C)', lambda: get_peak_temperature_traces(run), [0], yMin=45, yMax=100, force_recreate=force_recreate)
    plot_trace(run, 'utilization', 'Utilization', 'Utilization (%)', lambda: get_utilization_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'rel_nuca_cpi', 'Rel. NUCA CPI', 'Rel. NUCA CPI', lambda: get_rel_nuca_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'cpi', 'CPI', 'CPI', lambda: get_cpi_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    #plot_trace(run, 'abs_nuca_cpi', 'Abs. NUCA CPI', 'Abs. NUCA CPI', lambda: get_nuca_cpi_traces(run), active_cores, yMin=0, force_recreate=force_recreate)
    plot_trace(run, 'ips', 'IPS', 'IPS', lambda: get_ips_traces(run), active_cores, yMin=0, yMax=8e9, force_recreate=force_recreate)
    plot_trace(run, 'ipssmooth', 'Smoothed IPS', 'Smoothed IPS', lambda: get_ips_traces(run), active_cores, yMin=0, yMax=8e9, smooth=10, force_recreate=force_recreate)
    plot_cpi_stack_trace(run, active_cores, force_recreate=force_recreate)
    plot_hb_trace(run, force_recreate)
    plot_hb_histogram(run, force_recreate)

    # Make perforation statistics for the current run.
    perforation_statistics(run)


if __name__ == '__main__':
    for run in sorted(get_runs())[::-1]:
        create_plots(run)
