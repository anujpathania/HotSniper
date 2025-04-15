import datetime
import math
import os
import gzip
import platform
import random
import re
import shutil
import subprocess
import time
import traceback
import sys


from config import NUMBER_CORES, RESULTS_FOLDER, SNIPER_CONFIG, SCRIPTS, ENABLE_HEARTBEATS
from resultlib.plot import create_plots

HERE = os.path.dirname(os.path.abspath(__file__))
SNIPER_BASE = os.path.dirname(HERE)
BENCHMARKS = os.path.join(SNIPER_BASE, 'benchmarks')
BATCH_START = datetime.datetime.now().strftime('%Y-%m-%d_%H.%M')


def change_base_configuration(base_configuration):
    base_cfg = os.path.join(SNIPER_BASE, 'config/base.cfg')
    with open(base_cfg, 'r') as f:
        content = f.read()
    with open(base_cfg, 'w') as f:
        for line in content.splitlines():
            m = re.match('.*cfg:(!?)([a-zA-Z_\\.0-9]+)$', line)
            if m:
                inverted = m.group(1) == '!'
                include = inverted ^ (m.group(2) in base_configuration)
                included = line[0] != '#'
                if include and not included:
                    line = line[1:]
                elif not include and included:
                    line = '#' + line
            f.write(line)
            f.write('\n')


def prev_run_cleanup():
    '''Cleanup files potentially left over from aborted previous runs.'''

    pattern = r"^\d+\.hb.log$" # Heartbeat logs
    for f in os.listdir(BENCHMARKS):
        if not re.match(pattern, f):
            continue

        file_path = os.path.join(BENCHMARKS, f)
        if os.path.isfile(file_path):
            try:
                os.remove(file_path)
            except OSError as e:
                print(f"Error removing {file_path}: {e}", file=sys.stderr)


    for f in os.listdir(BENCHMARKS):
        if ('output.' in f) or ('.264' in f) or ('poses.' in f) or ('app_mapping' in f) :
            try:
                os.remove(os.path.join(BENCHMARKS, f))
            except OSError as e:
                 print(f"Error removing {os.path.join(BENCHMARKS, f)}: {e}", file=sys.stderr)


def save_output(base_configuration, benchmark, console_output, cpistack, started, ended):
    benchmark_text = benchmark
    if len(benchmark_text) > 100:
        benchmark_text = benchmark_text[:100] + '__etc'
    run_id = 'results_{}_{}_{}'.format(BATCH_START, '+'.join(base_configuration), benchmark_text)
    directory = os.path.join(RESULTS_FOLDER, run_id)
    if not os.path.exists(directory):
        os.makedirs(directory)
    # Save console output
    with gzip.open(os.path.join(directory, 'execution.log.gz'), 'wt', encoding='utf-8') as f: # Use 'wt' for text mode
        f.write(console_output)
    # Save execution info
    with open(os.path.join(directory, 'executioninfo.txt'), 'w') as f:
        f.write('started:    {}\n'.format(started.strftime('%Y-%m-%d %H:%M:%S')))
        f.write('ended:      {}\n'.format(ended.strftime('%Y-%m-%d %H:%M:%S')))
        f.write('duration:   {}\n'.format(ended - started))
        f.write('host:       {}\n'.format(platform.node()))
        f.write('tasks:      {}\n'.format(benchmark))
        f.write('run_id:     {}\n'.format(run_id)) # Add run_id for easier identification
    # Save CPI stack text
    with open(os.path.join(directory, 'cpi-stack.txt'), 'wb') as f:
        f.write(cpistack)
    # Copy essential simulation files
    for f in ('sim.cfg',
              'sim.info',
              'sim.out',
              'cpi-stack.png',
              'sim.stats.sqlite3'):
        src_file = os.path.join(BENCHMARKS, f)
        if os.path.exists(src_file):
             try:
                shutil.copy(src_file, directory)
             except Exception as e:
                 print(f"Error copying {src_file} to {directory}: {e}", file=sys.stderr)
        else:
            print(f"Warning: File {src_file} not found, skipping copy.", file=sys.stderr)
    # Copy and compress periodic logs
    for f in ('PeriodicPower.log',
              'PeriodicThermal.log',
              'PeriodicFrequency.log',
              'PeriodicVdd.log',
              'PeriodicCPIStack.log',
              'PeriodicRvalue.log'):
        src_file = os.path.join(BENCHMARKS, f)
        dest_file_gz = '{}.gz'.format(os.path.join(directory, f))
        if os.path.exists(src_file):
            try:
                with open(src_file, 'rb') as f_in, gzip.open(dest_file_gz, 'wb') as f_out:
                    shutil.copyfileobj(f_in, f_out)
            except Exception as e:
                print(f"Error copying/compressing {src_file} to {dest_file_gz}: {e}", file=sys.stderr)
        else:
            print(f"Warning: File {src_file} not found, skipping copy/compression.", file=sys.stderr)

    # Copy heartbeat logs
    pattern = r"^\d+\.hb.log$" # Heartbeat logs
    for f in os.listdir(BENCHMARKS):
        if not re.match(pattern, f):
            continue
        src_file = os.path.join(BENCHMARKS, f)
        if os.path.exists(src_file):
            try:
                shutil.copy(src_file, directory)
            except Exception as e:
                 print(f"Error copying {src_file} to {directory}: {e}", file=sys.stderr)

    # Copy other output files (e.g., x264 specifics)
    for f in os.listdir(BENCHMARKS):
        if ('output.' in f) or ('poses.' in f) or ('.264' in f) or ('app_mapping.' in f) :
            src_file = os.path.join(BENCHMARKS, f)
            if os.path.exists(src_file):
                try:
                    shutil.copy(src_file, directory)
                except Exception as e:
                    print(f"Error copying {src_file} to {directory}: {e}", file=sys.stderr)

    # Attempt to create plots, handle potential errors
    try:
        create_plots(run_id)
    except Exception as e:
        print(f"Error creating plots for run {run_id}: {e}", file=sys.stderr)
        print(traceback.format_exc(), file=sys.stderr)


def run(base_configuration, benchmark, ignore_error=False, perforation_script: str = None):
    print('running {} with configuration {}'.format(benchmark, '+'.join(base_configuration)))
    started = datetime.datetime.now()
    change_base_configuration(base_configuration)

    prev_run_cleanup()

    benchmark_options = []
    if ENABLE_HEARTBEATS == True:
        benchmark_options.append('enable_heartbeats')
        benchmark_options.append('hb_results_dir=%s' % BENCHMARKS)

    # NOTE: This determines the logging interval! (see issue in forked repo)
    periodicPower = 1000000 # Default: 1ms
    #periodicPower = 250000 # 250us
    if 'mediumDVFS' in base_configuration:
        periodicPower = 250000
    if 'fastDVFS' in base_configuration:
        periodicPower = 100000 # 100us

    if not perforation_script:
        # Default perforation script if none provided
        perforation_script = 'magic_perforation_rate:'

    # Construct arguments carefully
    args = [
        '-n', str(NUMBER_CORES),
        '-c', SNIPER_CONFIG,
        '--benchmarks={}'.format(benchmark),
        '--no-roi',
        '--sim-end=last',
        '-senergystats:{}'.format(periodicPower),
        '-speriodic-power:{}'.format(periodicPower),
    ]
    # Add scripts from config
    for s in SCRIPTS:
        args.append('-s' + s)
    # Add perforation script
    args.append('-s' + perforation_script)
    # Add benchmark options
    for opt in benchmark_options:
        args.append('-B')
        args.append(opt)

    console_output = ''

    print("Running sniper with args: ", ' '.join(args)) # Print the command line args

    run_sniper_path = os.path.join(BENCHMARKS, 'run-sniper')

    # Ensure run-sniper is executable
    if not os.access(run_sniper_path, os.X_OK):
        try:
            os.chmod(run_sniper_path, 0o755)
            print(f"Made {run_sniper_path} executable.")
        except OSError as e:
            print(f"Error changing permissions for {run_sniper_path}: {e}", file=sys.stderr)
            raise # Re-raise the error if permissions cannot be set


    p = subprocess.Popen([run_sniper_path] + args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=1, cwd=BENCHMARKS, text=True, encoding='utf-8') # Use text=True
    with p.stdout:
        for line in iter(p.stdout.readline, ''): # Iterate until empty string for EOF
            # linestr = line.decode('utf-8') # No need to decode if text=True
            console_output += line
            print(line, end='') # Already a string

    p.wait()

    try:
        # Ensure cpistack.py path is correct
        cpistack_script = os.path.join(SNIPER_BASE, 'tools/cpistack.py')
        if not os.path.exists(cpistack_script):
             print(f"Error: cpistack.py not found at {cpistack_script}", file=sys.stderr)
             raise FileNotFoundError("cpistack.py not found")
        cpistack = subprocess.check_output([sys.executable, cpistack_script], cwd=BENCHMARKS) # Use sys.executable
    except FileNotFoundError:
         if ignore_error:
             cpistack = b'cpistack.py not found'
         else:
             raise
    except subprocess.CalledProcessError as e:
        print(f"Error running cpistack.py: {e}", file=sys.stderr)
        if ignore_error:
            cpistack = f'Error running cpistack.py: {e}'.encode('utf-8')
        else:
            raise
    except Exception as e: # Catch other potential errors
        print(f"Unexpected error running cpistack.py: {e}", file=sys.stderr)
        if ignore_error:
            cpistack = f'Unexpected error running cpistack.py: {e}'.encode('utf-8')
        else:
            raise


    ended = datetime.datetime.now()

    save_output(base_configuration, benchmark, console_output, cpistack, started, ended)

    if p.returncode != 0 and not ignore_error:
        print(f"Error: Sniper exited with return code {p.returncode}", file=sys.stderr)
        raise Exception('return code != 0')


def try_run(base_configuration, benchmark, ignore_error=False, perforation_script: str = None):
    try:
        run(base_configuration, benchmark, ignore_error=ignore_error, perforation_script=perforation_script)
    except KeyboardInterrupt:
        print("\nRun interrupted by user.")
        raise # Re-raise to stop execution if desired
    except Exception as e:
        print('\n' + '#' * 80)
        print("### ERROR ENCOUNTERED ###")
        print(traceback.format_exc())
        print(f"Failed running benchmark: {benchmark} with config: {base_configuration}")
        print('#' * 80 + '\n')
        # Decide whether to continue or stop
        # input('Error encountered. Press Enter to continue with the next run, or Ctrl+C to abort...')


class Infeasible(Exception):
    """Custom exception for infeasible benchmark/parallelism combinations."""
    pass


def get_instance(benchmark, parallelism, input_set='small'):
    # Defines the valid parallelism (thread counts) for each benchmark.
    # 0 indicates infeasible for that specific count.
    threads = {
        # PARSEC Benchmarks
        'parsec-blackscholes': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-bodytrack': [1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Often starts at 3
        'parsec-canneal': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-dedup': [1, 0, 0, 4, 0, 0, 7, 0, 0, 10, 0, 0, 13, 0, 0, 16], # Specific thread counts
        'parsec-ferret': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Assuming 1-16 is possible
        'parsec-fluidanimate': [1, 2, 3, 0, 5, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0], # Limited parallelism
        'parsec-streamcluster': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-swaptions': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-vips': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Assuming 1-16 is possible
        'parsec-x264': [1, 0, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0], # Limited parallelism

        # SPLASH2 Benchmarks
        'splash2-barnes': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-cholesky': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-fft': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16], # Powers of 2
        'splash2-fmm': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-lu.cont': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-lu.ncont': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-ocean.cont': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16], # Powers of 2
        'splash2-ocean.ncont': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16], # Powers of 2
        'splash2-radiosity': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-radix': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16], # Powers of 2
        'splash2-raytrace': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-water.nsq': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-water.sp': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16], # Powers of 2
    }

    if benchmark not in threads:
        print(f"Warning: Benchmark '{benchmark}' not found in threads dictionary. Assuming 1-16 threads possible.", file=sys.stderr)
        ps = list(range(1, 17)) # Default assumption if benchmark missing
    else:
        ps = threads[benchmark]

    # Check if the requested parallelism is valid and non-zero in the list
    if parallelism <= 0 or parallelism > len(ps) or ps[parallelism-1] == 0:
         raise Infeasible(f"Parallelism level {parallelism} is not feasible for benchmark {benchmark}")

    # The index p for the benchmark name seems 1-based in the original script logic
    # Let's find the actual non-zero thread count at the requested index `parallelism`
    actual_thread_count = ps[parallelism-1]

    # The suffix '-p' seems determined by the position in the *valid* list, not the absolute number.
    # Let's find the 1-based index of the actual_thread_count in the list of *valid* counts.
    valid_ps = [p_val for p_val in ps if p_val > 0]
    try:
        p_suffix = valid_ps.index(actual_thread_count) + 1
    except ValueError:
        # This should not happen if the initial check passed, but handle defensively
        raise Infeasible(f"Could not determine suffix for benchmark {benchmark} with parallelism {parallelism}")


    if benchmark.startswith('parsec') and not input_set.startswith('sim'):
        input_set = 'sim' + input_set

    # Construct the instance name, e.g., parsec-blackscholes-simsmall-p2
    return '{}-{}-p{}'.format(benchmark, input_set, p_suffix)


def get_feasible_parallelisms(benchmark):
    """Returns a list of feasible parallelism levels (thread counts) for a given benchmark."""
    feasible = []
    # Check up to 16 threads (or adjust if needed)
    for p in range(1, 16 + 1):
        try:
            # Try to get the instance - this will check feasibility internally
            get_instance(benchmark, p)
            feasible.append(p)
        except Infeasible:
            pass # This parallelism level is not feasible
    return feasible


def get_workload(benchmark, cores, parallelism=None, number_tasks=None, input_set='small'):
    # This function seems designed for scheduling multiple tasks, potentially complex.
    # Keeping original logic, but may need adjustment based on exact use case.
    if parallelism is not None:
        number_tasks = math.floor(cores / parallelism)
        return get_workload(benchmark, cores, number_tasks=number_tasks, input_set=input_set)
    elif number_tasks is not None:
        if number_tasks == 0:
            if cores == 0:
                return []
            else:
                raise Infeasible("Cannot have 0 tasks with >0 cores")
        else:
            # Try to find a feasible parallelism level p for the remaining cores/tasks
            target_parallelism = math.ceil(cores / number_tasks)
            found = False
            # Iterate downwards from the target parallelism
            for p in reversed(range(1, min(cores, target_parallelism) + 1)):
                try:
                    b = get_instance(benchmark, p, input_set=input_set)
                    # Recursively call for remaining cores and tasks
                    remaining_workload = get_workload(benchmark, cores - p, number_tasks=number_tasks-1, input_set=input_set)
                    return [b] + remaining_workload
                except Infeasible:
                    continue # Try the next lower parallelism level
            # If no feasible p was found in the loop
            raise Infeasible(f"Could not find feasible parallelism for {number_tasks} tasks on {cores} cores for {benchmark}")
    else:
        raise ValueError('Either parallelism or number_tasks needs to be set')


def example():
    # Original example function - kept for reference
    for benchmark in (
                      'parsec-blackscholes',
                      #'parsec-bodytrack',
                      # ... other benchmarks ...
                      ):

        min_parallelism = get_feasible_parallelisms(benchmark)[0]
        max_parallelism = get_feasible_parallelisms(benchmark)[-1]
        for freq in (1, 2):
            #for parallelism in (max_parallelism,):
            for parallelism in (3, ):
                try:
                     # Use try_run for robustness
                    try_run(['{:.1f}GHz'.format(freq), 'maxFreq', 'slowDVFS'], get_instance(benchmark, parallelism, input_set='simsmall'))
                except Infeasible as e:
                    print(f"Skipping infeasible run: {e}")
                except Exception as e:
                    print(f"Error during example run: {e}")


def run_multithreading_experiments():
    """Runs experiments for Assignment 2, Section 3: Multi-Threading."""
    benchmarks_to_run = [
        'parsec-blackscholes',
        'parsec-streamcluster',
    ]
    thread_counts = [1, 2, 3, 4] # Test threads 1 through 4
    # Use a fixed configuration for comparability
    base_configuration = ['4.0GHz', 'maxFreq', 'slowDVFS']
    input_set = 'simsmall' # Use simsmall input set

    print(f"\n--- Starting Multi-Threading Experiments ({BATCH_START}) ---")
    print(f"Target Benchmarks: {', '.join(benchmarks_to_run)}")
    print(f"Target Thread Counts: {thread_counts}")
    print(f"Base Configuration: {base_configuration}")
    print(f"Input Set: {input_set}")
    print("-" * 60)


    for benchmark in benchmarks_to_run:
        print(f"\n--- Running Benchmark: {benchmark} ---")
        try:
            feasible_threads = get_feasible_parallelisms(benchmark)
            print(f"Feasible thread counts for {benchmark}: {feasible_threads}")
        except KeyError:
            print(f"Error: Benchmark '{benchmark}' not defined in get_instance dictionary. Skipping.", file=sys.stderr)
            continue # Skip to the next benchmark


        for n_threads in thread_counts:
            # Check if the desired thread count is actually supported by the benchmark
            if n_threads not in feasible_threads:
                print(f"\nSkipping {benchmark} with {n_threads} threads (infeasible).")
                continue

            print(f"\n-- Running {benchmark} with {n_threads} thread(s) --")
            try:
                # Construct the benchmark instance name (e.g., parsec-blackscholes-simsmall-p1)
                instance_name = get_instance(benchmark, n_threads, input_set=input_set)
                # Use try_run for better error handling during batch runs
                try_run(base_configuration, instance_name, ignore_error=True) # ignore_error=True allows the batch to continue
            except Infeasible as e:
                # This case should ideally be caught by the check above, but added for safety
                print(f"Caught Infeasible exception: {e}. Skipping run.", file=sys.stderr)
            except Exception as e:
                # Catch any other unexpected errors during the run setup or execution
                print(f"An unexpected error occurred while running {benchmark} with {n_threads} threads:", file=sys.stderr)
                print(traceback.format_exc(), file=sys.stderr)
                print("Continuing with the next experiment...")

        print(f"\n--- Finished Benchmark: {benchmark} ---")

    print("\n--- Completed Multi-Threading Experiments ---")
    print("-" * 60 + "\n")



def multi_program():
    # Original function - kept for reference
    input_set = 'simsmall'
    base_configuration = ['4.0GHz', "maxFreq"]
    benchmark_set = (
        'parsec-blackscholes',
        'parsec-x264',
    )

    if ENABLE_HEARTBEATS == True:
        base_configuration.append('hb_enabled')

    benchmarks = ''
    for i, benchmark in enumerate(benchmark_set):
        try:
            min_parallelism = get_feasible_parallelisms(benchmark)[0]
            instance = get_instance(benchmark, min_parallelism, input_set)
            if i != 0:
                benchmarks = benchmarks + ',' + instance
            else:
                benchmarks = benchmarks + instance
        except (IndexError, Infeasible, KeyError) as e:
             print(f"Error getting instance for {benchmark} in multi_program: {e}. Skipping.", file=sys.stderr)
             return # Abort this multi_program run if any benchmark fails

    if benchmarks: # Only run if benchmarks were successfully constructed
        try_run(base_configuration, benchmarks, ignore_error=True)


def test_static_power():
     # Original function - kept for reference
    try:
        try_run(['4.0GHz', 'testStaticPower', 'slowDVFS'], get_instance('parsec-blackscholes', 3, input_set='simsmall'), ignore_error=True)
    except Infeasible as e:
        print(f"Skipping test_static_power: {e}")

def ondemand_demo():
    # Original function - kept for reference
    try:
        try_run(['4.0GHz', 'ondemand', 'fastDVFS'], get_instance('parsec-blackscholes', 3, input_set='simsmall'), ignore_error=True)
    except Infeasible as e:
         print(f"Skipping ondemand_demo: {e}")


def coldestcore_demo():
     # Original function - kept for reference
    try:
        try_run(['2.4GHz', 'maxFreq', 'slowDVFS', 'coldestCore'], get_instance('parsec-blackscholes', 3, input_set='simsmall'), ignore_error=True)
    except Infeasible as e:
         print(f"Skipping coldestcore_demo: {e}")


def main():
    # Comment out the demos/examples you don't want to run
    # example()
    # ondemand_demo()
    # coldestcore_demo()
    # test_static_power()
    # multi_program()

    # --- Run the Assignment 2 Multi-Threading Experiments ---
    run_multithreading_experiments()
    # --------------------------------------------------------


if __name__ == '__main__':
    main()