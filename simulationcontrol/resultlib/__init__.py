import diskcache
import gzip
import io
import os
import re
try:
    from config import RESULTS_FOLDER
except ImportError:
    from ..config import RESULTS_FOLDER

HERE = os.path.dirname(os.path.abspath(__file__))
RESULT_DIRS = [RESULTS_FOLDER]
NAME_REGEX = r'results_(\d+-\d+-\d+_\d+.\d+)_([a-zA-Z0-9_\.\+]+)_((splash2|parsec)-.*)'

print(NAME_REGEX)

cache = diskcache.Cache(directory=os.path.join(HERE, 'cache'))


def get_runs():
    for result_dir in RESULT_DIRS:
        if os.path.exists(result_dir):
            for dirname in os.listdir(result_dir):
                if dirname.startswith('results_'):
                    yield dirname


def find_run(run):
    for result_dir in RESULT_DIRS:
        candidate = os.path.join(result_dir, run)
        if os.path.exists(candidate):
            return candidate
    raise Exception('could not find run')


def _open_file(run, filename):
    for base_dir in RESULT_DIRS:
        full_filename = os.path.join(base_dir, run, filename)
        if os.path.exists(full_filename):
            return open(full_filename, 'r', encoding="utf-8")

        gzip_filename = '{}.gz'.format(full_filename)
        if os.path.exists(gzip_filename):
            return io.TextIOWrapper(gzip.open(gzip_filename, 'r'), encoding="utf-8")
    raise Exception('file does not exist')


def get_date(run):
    m = re.search(NAME_REGEX, run)
    return m.group(1)

def get_config(run):
    m = re.search(NAME_REGEX, run)
    return m.group(2)


def get_tasks(run):
    m = re.search(NAME_REGEX, run)
    tasks = m.group(3).split(',')
    return ', '.join(t[t.find('-')+1:] for t in tasks)


def has_properly_finished(run):
    return get_average_response_time(run) is not None


@cache.memoize()
def get_total_simulation_time(run):
    with _open_file(run, 'sim.out') as f:
        for line in f:
            tokens = line.split()
            if tokens[0] == 'Time':
                return int(tokens[3])
    return '-'


@cache.memoize()
def get_average_response_time(run):
    with _open_file(run, 'execution.log') as f:
        for line in f:
            m = re.search(r'Average Response Time \(ns\)\s+:\s+(\d+)', line)
            if m is not None:
                return int(m.group(1))


@cache.memoize()
def get_individual_response_times(run):
    resp_times = {}
    with _open_file(run, 'execution.log') as f:
        for line in f:
            m = re.search(r'Task (\d+) \(Response/Service/Wait\) Time \(ns\)\s+:\s+(\d+)\s+(\d+)\s+(\d+)', line)
            if m is not None:
                task = int(m.group(1))
                resp = int(m.group(2))
                resp_times[task] = resp
    keys = sorted(resp_times.keys())
    if len(keys) == 0:
        return '-'
    elif keys != list(range(max(keys)+1)):
        raise Exception('task(s) missing: {}'.format(', '.join(map(str, sorted(list(set(range(max(keys)+1)) - set(keys)))))))
    else:
        return [resp_times[task] for task in keys]


@cache.memoize()
def get_individual_start_times(run):
    start_times = {}
    with _open_file(run, 'execution.log') as f:
        for line in f:
            m = re.search(r'\[Scheduler\]: Trying to schedule Task (?P<task>\d+) at Time (?P<time>\d+) ns', line)
            if m is not None:
                task = int(m.group('task'))
                t = int(m.group('time'))
                start_times[task] = t
    start_keys = sorted(start_times.keys())
    if start_keys == []:
        raise Exception('task(s) missing (non found)')
    elif start_keys != list(range(max(start_keys)+1)):
        raise Exception('task(s) missing')
    else:
        return [start_times[task] for task in start_keys]


@cache.memoize()
def get_individual_energies(run):
    """
    get energy consumption per task
    """
    start_times = get_individual_start_times(run)
    response_times = get_individual_response_times(run)
    end_times = [start_times[i] + response_times[i] for i in range(len(start_times))]
    traces = get_power_traces(run)
    power_values = [sum(ps) for ps in zip(*traces)]

    energies = [0] * len(start_times)

    dt = 100000
    for i, p in enumerate(power_values):
        t = i * dt
        active_tasks = [i for i in range(len(start_times)) if start_times[i] <= t and end_times[i] > t]
        for t in active_tasks:
            energies[t] += p / len(active_tasks) * dt / 1e9

    # double check
    assert abs(sum(energies) - get_energy(run)) < 0.01, '{:.2f} != {:.2f}'.format(sum(energies), get_energy(run))
    return energies


@cache.memoize()
def get_average_power_consumption(run):
    traces = get_power_traces(run)
    power_values = [sum(ps) for ps in zip(*traces)]
    return avg(power_values)


@cache.memoize()
def get_peak_power_consumption_single_thread(run):
    traces = get_power_traces(run)
    return max(max(trace) for trace in traces)


@cache.memoize()
def get_energy(run):
    power = get_average_power_consumption(run)
    simulation_time = get_total_simulation_time(run)
    if simulation_time in ('-',):
        return '-'
    time = simulation_time / 1e9
    return power * time


def avg(data):
    return float(sum(data)) / len(data)


def _get_traces(run, filename, multiplicator=1):
    traces = []

    with _open_file(run, filename) as f:
        f.readline()
        for line in f:
            vs = [multiplicator * float(v) for v in line.split()]
            traces.append(vs)

    return list(zip(*traces))


def get_power_traces(run):
    return _get_traces(run, 'PeriodicPower.log')


def get_temperature_traces(run):
    return _get_traces(run, 'PeriodicThermal.log')


def get_peak_temperature_traces(run):
    traces = get_temperature_traces(run)
    peak = []
    for values in zip(*traces):
        peak.append(max(values))
    return [peak]


@cache.memoize()
def get_average_peak_temperature(run):
    trace = get_peak_temperature_traces(run)[0]
    return avg(trace)


@cache.memoize()
def get_rel_duration_temperature_violation(run, max_temperature=80):
    trace = get_peak_temperature_traces(run)[0]
    return sum(1 for t in trace if t > max_temperature) / len(trace)


@cache.memoize()
def get_area_temperature_violation(run, max_temperature=80):
    trace = get_peak_temperature_traces(run)[0]
    return sum(t - max_temperature for t in trace if t > max_temperature) / len(trace)


@cache.memoize()
def get_rel_duration_temperature_violation_per_core(run, max_temperature=80):
    traces = get_temperature_traces(run)
    violations = []
    for trace in traces:
        violations.append(sum(1 for t in trace if t > max_temperature) / len(trace))
    return violations


def exists_full_freq_trace(run):
    try:
        with _open_file(run, 'PeriodicFrequency.log') as f:
            return True
    except:
        return False


@cache.memoize()
def get_cpi_stack_trace_parts(run):
    parts = []
    with _open_file(run, 'PeriodicCPIStack.log') as f:
        f.readline()
        for line in f:
            part = line.split()[0]
            if part not in parts:
                parts.append(part)
    return parts


def get_cpi_stack_part_trace(run, part='total'):
    traces = []

    trace_values = []
    with _open_file(run, 'PeriodicCPIStack.log') as f:
        f.readline()
        for line in f:
            if line.startswith(part + '\t'):
                items = line.split()[1:]
                if items == ['-']:
                    ps = [0] * count_cores(run)
                else:
                    ps = [float(value) for value in items]
                trace_values.append(ps)

    return list(zip(*trace_values))


def _add_traces(trace1, trace2):
    assert len(trace1) == len(trace2), 'number of cores differs: {} != {}'.format(len(trace1), len(trace2))
    traces = []
    for t1, t2 in zip(trace1, trace2):
        assert len(t1) == len(t2), 'length of traces differ: {} != {}'.format(len(t1), len(t2))
        traces.append([v1 + v2 for v1, v2 in zip(t1, t2)])
    return traces


def _divide_traces(numerator, denominator):
    assert len(numerator) == len(denominator), 'number of cores differs: {} != {}'.format(len(numerator), len(denominator))
    traces = []
    for num, den in zip(numerator, denominator):
        assert len(num) == len(den), 'length of traces differ: {} != {}'.format(len(num), len(den))
        traces.append([n / d if d != 0 else None for n, d in zip(num, den)])
    return traces


def get_ips_traces(run):
    return _divide_traces(get_freq_traces(run), get_cpi_traces(run, raw=True))


def get_cpi_traces(run, raw=False):
    traces = list(map(list, get_cpi_stack_part_trace(run, 'total')))
    if not raw:
        w = 2
        for trace in traces:
            drop = [i for i in range(len(trace)) if any(t > 20 for t in trace[max(i-w,0):min(i+w,len(trace))])]
            for i in drop:
                trace[i] = None
    return traces


def get_nuca_cpi_traces(run):
    return _add_traces(get_cpi_stack_part_trace(run, 'mem-nuca'), get_cpi_stack_part_trace(run, 'ifetch'))


def get_freq_traces(run):
    return _get_traces(run, 'PeriodicFrequency.log', multiplicator=1e9)


@cache.memoize()
def get_max_freq(run):
    return max(max(t) for t in get_freq_traces(run))


def get_rel_nuca_traces(run):
    return _divide_traces(get_nuca_cpi_traces(run), get_cpi_stack_part_trace(run, 'total'))


def get_utilization_traces(run):
    return _divide_traces(get_cpi_stack_part_trace(run, 'base'), get_cpi_stack_part_trace(run, 'total'))


@cache.memoize()
def count_cores(run):
    return len(get_power_traces(run))


@cache.memoize()
def get_active_cores(run):
    utilization_traces = get_utilization_traces(run)
    return [i for i, utilization in enumerate(utilization_traces) if max(utilization) > 0.01]


@cache.memoize()
def get_average_system_utilization(run):
    from .replay import Execution
    ex = Execution(run)
    ex.replay()
    return ex.average_system_utilization


@cache.memoize()
def get_peak_system_utilization(run):
    from .replay import Execution
    ex = Execution(run)
    ex.replay()
    return ex.peak_system_utilization
