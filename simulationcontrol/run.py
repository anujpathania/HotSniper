#!/usr/bin/env python3
# -*- coding: utf-8 -*-

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

# --- Configuration Import ---
# Attempt to import from config.py, provide defaults or exit if it fails
try:
    from config import NUMBER_CORES, RESULTS_FOLDER, SNIPER_CONFIG, SCRIPTS, ENABLE_HEARTBEATS
except ImportError:
    print("Error: config.py not found or missing required variables.", file=sys.stderr)
    print("Using default values, but results might be unexpected.", file=sys.stderr)
    NUMBER_CORES = 4
    RESULTS_FOLDER = './results'
    SNIPER_CONFIG = 'gainestown' # Example config, MAKE SURE THIS IS CORRECT for your setup
    SCRIPTS = []
    ENABLE_HEARTBEATS = False
    # Attempt to create results folder if using defaults
    if not os.path.exists(RESULTS_FOLDER):
        try:
            os.makedirs(RESULTS_FOLDER)
            print("Created default results folder: {}".format(RESULTS_FOLDER))
        except OSError as e:
            print("Fatal Error: Could not create default results folder {}: {}".format(RESULTS_FOLDER, e), file=sys.stderr)
            sys.exit(1)

# --- Plotting Import ---
# Attempt to import plotting function, provide dummy if it fails
try:
    # Ensure resultlib is accessible (e.g., in PYTHONPATH or installed)
    from resultlib.plot import create_plots
except ImportError:
    print("Warning: resultlib.plot not found. Plot generation will be skipped.", file=sys.stderr)
    # Define a dummy function
    def create_plots(run_name):
        # Use .format() for compatibility
        print("Skipping plot generation for {} (resultlib not found).".format(run_name))

# --- Global Constants ---
HERE = os.path.dirname(os.path.abspath(__file__))
# Assume simulationcontrol is one level down from the main sniper directory
SNIPER_BASE = os.path.dirname(HERE)
BENCHMARKS = os.path.join(SNIPER_BASE, 'benchmarks')
# Check if SNIPER_BASE seems correct
if not os.path.isdir(os.path.join(SNIPER_BASE, 'tools')):
     print("Warning: SNIPER_BASE ({}) might be incorrect. 'tools' directory not found.".format(SNIPER_BASE), file=sys.stderr)

# Use a consistent timestamp format suitable for filenames
BATCH_START = datetime.datetime.now().strftime('%Y-%m-%d_%H%M%S')

# --- Helper Functions ---

def change_base_configuration(base_configuration):
    """
    Modifies the config/base.cfg file to include/exclude specific configs
    by commenting/uncommenting 'include = "config/..."' lines.
    """
    base_cfg = os.path.join(SNIPER_BASE, 'config/base.cfg')
    # Use .format() for compatibility
    print("Updating base configuration: {}".format(base_cfg))
    print("Target active configs: {}".format(base_configuration))

    try:
        with open(base_cfg, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except IOError as e:
        print("Fatal Error: Cannot read base config {}: {}".format(base_cfg, e), file=sys.stderr)
        raise # Re-raise error, cannot proceed

    new_lines = []
    updated_configs = set()
    changed = False

    include_pattern = re.compile(r'^(#)?(\s*include\s*=\s*\"config/)(.*?)(\".*)$')
                                  # Grp 1: Optional '#'
                                  # Grp 2: Prefix ' include = "config/'
                                  # Grp 3: The actual config name (e.g., 'gainestown.cfg' or 'scheduler/ondemand.cfg')
                                  # Grp 4: Suffix '"' potentially followed by comments

    for line in lines:
        match = include_pattern.match(line)
        if match:
            is_commented = match.group(1) == '#'
            prefix = match.group(2)
            config_name = match.group(3).strip() # Get the config name
            suffix = match.group(4)

            # Determine if this specific config should be active
            should_be_active = config_name in base_configuration
            is_active = not is_commented

            # Track which configs we've processed to report missing ones later
            if config_name in base_configuration:
                updated_configs.add(config_name)

            if should_be_active and not is_active:
                # Uncomment the line (remove leading #)
                new_line = prefix + config_name + suffix + '\n' # Reconstruct without #
                new_lines.append(new_line)
                print("  Activating: {}".format(config_name))
                changed = True
            elif not should_be_active and is_active:
                # Comment the line (add #)
                new_line = '#' + prefix + config_name + suffix + '\n' # Add # at the beginning
                new_lines.append(new_line)
                print("  Deactivating: {}".format(config_name))
                changed = True
            else:
                # Line is already in the correct state
                new_lines.append(line.rstrip('\r\n') + '\n') # Add original line (ensuring one newline)
        else:
            # Keep non-include lines as they are
            new_lines.append(line.rstrip('\r\n') + '\n') # Ensure consistent newlines

    # Report any target configs not found in base.cfg
    missing_configs = set(base_configuration) - updated_configs
    if missing_configs:
        print("Warning: The following target configurations were not found as 'include=' lines in {}: {}".format(base_cfg, missing_configs), file=sys.stderr)

    if changed:
        print("Base configuration modified. Writing changes.")
        try:
            with open(base_cfg, 'w', encoding='utf-8') as f:
                f.writelines(new_lines)
        except IOError as e:
            print("Fatal Error: Cannot write updated base config {}: {}".format(base_cfg, e), file=sys.stderr)
            raise # Re-raise error
    else:
        print("Base configuration appears up-to-date. No changes written.")


def prev_run_cleanup():
    """Cleans up files potentially left over from aborted previous runs in BENCHMARKS dir."""
    print("--- Cleaning up previous run artifacts in {} ---".format(BENCHMARKS))
    if not os.path.isdir(BENCHMARKS):
        print("Warning: Benchmarks directory '{}' not found. Skipping cleanup.".format(BENCHMARKS), file=sys.stderr)
        return

    # Combined list of patterns for files to remove from BENCHMARKS directory
    patterns_to_remove = [
        r"^\d+\.hb.log$",           # Heartbeat logs (e.g., 0.hb.log)
        r"^output\..*",             # Output files (e.g., output.bodytrack.0)
        r".*\.264$",                # x264 specific output
        r"^poses\..*",              # Bodytrack specific output
        r"^app_mapping\..*",        # Mapping files
        r"^sim\.out$",              # Sniper output
        r"^sim\.info$",             # Sniper info
        r"^sim\.cfg$",              # Sniper config copy
        r"^sim\.stats.*",           # Sniper stats (including .sqlite3)
        r"^Periodic.*\.log(\.gz)?$",# Periodic logs (allow .gz just in case)
        r"^cpi-stack\.txt$",        # CPI stack text
        r"^cpi-stack\.png$",        # CPI stack image
    ]

    files_removed_count = 0
    for f in os.listdir(BENCHMARKS):
        file_path = os.path.join(BENCHMARKS, f)
        # Important: Only remove files, not directories
        if not os.path.isfile(file_path):
            continue

        should_remove = False
        for pattern in patterns_to_remove:
            if re.match(pattern, f):
                should_remove = True
                break

        if should_remove:
            try:
                os.remove(file_path)
                # print("Removed: {}".format(file_path)) # Enable for verbose logging
                files_removed_count += 1
            except OSError as e:
                # Use .format() for compatibility
                print("Warning: Error removing {}: {}".format(file_path, e), file=sys.stderr)

    print("--- Cleanup complete ({} files removed) ---".format(files_removed_count))


def save_output(base_configuration, benchmark, console_output, cpistack, started, ended):
    """Saves the simulation results to a structured directory."""
    print("--- Saving output ---")
    # Make benchmark name filename-safe (replace commas from multi-program)
    benchmark_text = benchmark.replace(',', '_').replace('/', '_')
    if len(benchmark_text) > 100: # Limit length
        benchmark_text = benchmark_text[:100] + '__etc'

    # Make config name filename-safe
    config_text = '_'.join(base_configuration).replace('+', '_plus_').replace('.', 'p').replace('/','_')
    run_name = 'results_{}_{}_{}'.format(BATCH_START, config_text, benchmark_text)
    directory = os.path.join(RESULTS_FOLDER, run_name)

    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
        print("Results directory: {}".format(directory))
    except OSError as e:
        print("Fatal Error: Could not create results directory {}: {}".format(directory, e), file=sys.stderr)
        return # Cannot save output if directory fails

    # --- Save Execution Log (Compressed Text) ---
    log_gz_path = os.path.join(directory, 'execution.log.gz')
    try:
        # Use 'wt' for writing text to gzip in Python 3
        with gzip.open(log_gz_path, 'wt', encoding='utf-8') as f:
            f.write(console_output)
    except Exception as e:
        print("Warning: Failed to save execution log {}: {}".format(log_gz_path, e), file=sys.stderr)

    # --- Save Execution Info ---
    info_path = os.path.join(directory, 'executioninfo.txt')
    try:
        with open(info_path, 'w', encoding='utf-8') as f:
            f.write('started:    {}\n'.format(started.strftime('%Y-%m-%d %H:%M:%S')))
            f.write('ended:      {}\n'.format(ended.strftime('%Y-%m-%d %H:%M:%S')))
            f.write('duration:   {}\n'.format(ended - started))
            f.write('host:       {}\n'.format(platform.node()))
            f.write('tasks:      {}\n'.format(benchmark))
            f.write('config:     {}\n'.format('+'.join(base_configuration)))
            f.write('python_ver: {}\n'.format(platform.python_version()))
    except IOError as e:
        print("Warning: Failed to save execution info {}: {}".format(info_path, e), file=sys.stderr)

    # --- Save CPI Stack Text ---
    cpi_txt_path = os.path.join(directory, 'cpi-stack.txt')
    try:
        # cpistack is likely bytes from subprocess, decode it
        cpistack_str = cpistack.decode('utf-8', 'replace') # Replace errors on decode
        with open(cpi_txt_path, 'w', encoding='utf-8') as f:
            f.write(cpistack_str)
    except Exception as e:
        # Catch potential decoding errors or IOErrors
        print("Warning: Failed to save cpi-stack.txt {}: {}".format(cpi_txt_path, e), file=sys.stderr)

    # --- Copy essential simulation output files (sim.*, cpi-stack.png) ---
    essential_files = ('sim.cfg', 'sim.info', 'sim.out', 'cpi-stack.png', 'sim.stats.sqlite3')
    for f in essential_files:
        src_path = os.path.join(BENCHMARKS, f)
        dest_path = os.path.join(directory, f)
        if os.path.exists(src_path):
            try:
                shutil.copy(src_path, dest_path)
            except IOError as e:
                 print("Warning: Could not copy file {} to {}: {}".format(src_path, dest_path, e), file=sys.stderr)
        # REMOVED the problematic 'pass' here. No action needed if source doesn't exist.

    # --- Copy and compress periodic logs ---
    periodic_logs = ('PeriodicPower.log', 'PeriodicThermal.log', 'PeriodicFrequency.log',
                     'PeriodicVdd.log', 'PeriodicCPIStack.log', 'PeriodicRvalue.log')
    for f in periodic_logs:
        src_path = os.path.join(BENCHMARKS, f)
        # Create destination path with .gz extension
        dest_gz_path = os.path.join(directory, f + '.gz')
        if os.path.exists(src_path):
            try:
                # Use binary read ('rb') and binary write ('wb') for gzip
                with open(src_path, 'rb') as f_in, gzip.open(dest_gz_path, 'wb') as f_out:
                    shutil.copyfileobj(f_in, f_out)
            except IOError as e:
                print("Warning: Could not copy/compress file {} to {}: {}".format(src_path, dest_gz_path, e), file=sys.stderr)

    # --- Copy Heartbeat logs (if any) ---
    hb_pattern = r"^\d+\.hb.log$"
    if os.path.isdir(BENCHMARKS):
        try:
            for f in os.listdir(BENCHMARKS):
                if not re.match(hb_pattern, f):
                    continue
                src_path = os.path.join(BENCHMARKS, f)
                dest_path = os.path.join(directory, f)
                if os.path.isfile(src_path): # Double-check it's a file
                    try:
                        shutil.copy(src_path, dest_path)
                    except IOError as e:
                        print("Warning: Could not copy heartbeat log {} to {}: {}".format(src_path, dest_path, e), file=sys.stderr)
        except OSError as e:
            print("Warning: Could not list benchmark directory for Heartbeat logs: {}".format(e), file=sys.stderr)

    # --- Copy Other Benchmark Specific Output (if any) ---
    bench_specific_patterns = ['output.', 'poses.', '.264', 'app_mapping.']
    if os.path.isdir(BENCHMARKS):
         try:
            for f in os.listdir(BENCHMARKS):
                copy_file = False
                for pattern in bench_specific_patterns:
                    # Use 'in' for substring check, might need more specific matching if names clash
                    if pattern in f:
                        copy_file = True
                        break
                if copy_file:
                    src_path = os.path.join(BENCHMARKS, f)
                    dest_path = os.path.join(directory, f)
                    if os.path.isfile(src_path):
                        try:
                            shutil.copy(src_path, dest_path)
                        except IOError as e:
                            print("Warning: Could not copy benchmark output {} to {}: {}".format(src_path, dest_path, e), file=sys.stderr)
         except OSError as e:
            print("Warning: Could not list benchmark directory for specific output: {}".format(e), file=sys.stderr)

    # --- Create Plots (Optional, requires resultlib) ---
    try:
        # Check if function exists and is callable before calling
        if 'create_plots' in globals() and callable(create_plots):
             print("--- Generating plots ---")
             create_plots(run_name) # Pass the generated run_name
             print("--- Plot generation complete (or skipped if unavailable) ---")
        else:
             print("--- Skipping plot generation (create_plots function unavailable) ---")
    except Exception as e:
        # Catch any error during plotting
        print("="*20 + " WARNING: Plot Generation Failed " + "="*20, file=sys.stderr)
        print("Failed to generate plots for run: {}".format(run_name), file=sys.stderr)
        traceback.print_exc(file=sys.stderr) # Print traceback to stderr
        print("="*70, file=sys.stderr)

    print("--- Output saving complete for {} ---".format(run_name))


def run(base_configuration, benchmark, ignore_error=False, perforation_script=None):
    """Runs a single Sniper simulation instance."""
    # Use print function, .format()
    print('\n' + '*' * 80)
    print('*** Running Benchmark: [{}], Config: [{}] ***'.format(benchmark, '+'.join(base_configuration)))
    print('*' * 80)
    started = datetime.datetime.now()
    try:
        change_base_configuration(base_configuration)
    except Exception as e:
        print("Fatal Error: Failed to change base configuration: {}".format(e), file=sys.stderr)
        if not ignore_error: raise
        else: return # Exit run if config fails and ignoring errors

    prev_run_cleanup()

    benchmark_options = []
    if ENABLE_HEARTBEATS:
        benchmark_options.append('enable_heartbeats')
        benchmark_options.append('hb_results_dir={}'.format(BENCHMARKS))

    # Determine periodic logging interval based on config
    periodicPower = 1000000 # Default 1ms (1,000,000 ns)
    if 'mediumDVFS' in base_configuration:
        periodicPower = 250000 # 0.25ms
    if 'fastDVFS' in base_configuration:
        periodicPower = 100000 # 0.1ms
    print("Using periodic logging interval: {} ns".format(periodicPower))

    # Format perforation script argument
    perforation_arg = ' -s{}'.format(perforation_script) if perforation_script else ''

    # Construct Sniper arguments using .format()
    args_template = ('-n {number_cores} -c {config} --benchmarks={benchmark} '
                     '--no-roi --sim-end=last -senergystats:{periodic} '
                     '-speriodic-power:{periodic}{script}{perforation}{benchmark_options}')
    args = args_template.format(
                number_cores=NUMBER_CORES,
                config=SNIPER_CONFIG,
                benchmark=benchmark,
                periodic=periodicPower,
                script= ''.join([' -s' + s for s in SCRIPTS]),
                perforation=perforation_arg,
                benchmark_options=''.join([' -B ' + opt for opt in benchmark_options]))

    console_output_list = [] # Store lines for final log

    print("Executing sniper command:")
    run_sniper = os.path.join(BENCHMARKS, 'run-sniper')
    cmd_list = [run_sniper] + args.split(' ')
    print(" ".join(cmd_list)) # Print the command being run

    # Check run-sniper script
    if not (os.path.isfile(run_sniper) and os.access(run_sniper, os.X_OK)):
         msg = "Fatal Error: run-sniper script not found or not executable at: {}".format(run_sniper)
         print(msg, file=sys.stderr)
         raise RuntimeError(msg)

    p = None # Initialize process variable
    try:
        # Start the Sniper process
        p = subprocess.Popen(cmd_list,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, # Redirect stderr to stdout
                             bufsize=1, # Line buffered
                             cwd=BENCHMARKS,
                             universal_newlines=False) # Read as bytes

        # Read output line by line while process runs
        print("--- Sniper Output Start ---")
        while True:
            line_bytes = p.stdout.readline()
            if not line_bytes: # End of stream
                break
            try:
                # Decode bytes to string, replacing errors
                line_str = line_bytes.decode('utf-8', 'replace')
            except Exception as decode_err:
                print("Warning: Error decoding line: {}".format(decode_err), file=sys.stderr)
                line_str = "[Decode Error] " + repr(line_bytes) + "\n"

            console_output_list.append(line_str) # Store decoded line
            sys.stdout.write(line_str) # Write to console immediately
            sys.stdout.flush()

        print("--- Sniper Output End ---")
        p.wait() # Wait for the process to complete

    except FileNotFoundError as fnf_err:
         print("\nFatal Error: Command not found (is run-sniper or a dependency missing?): {}".format(fnf_err), file=sys.stderr)
         if p: p.kill() # Attempt to kill if process started
         if not ignore_error: raise
         else: return
    except Exception as popen_err:
         print("\nFatal Error: Failed to start or run Sniper process: {}".format(popen_err), file=sys.stderr)
         if p: p.kill() # Attempt to kill if process started
         if not ignore_error: raise
         else: return

    # --- Process finished, check return code ---
    return_code = p.returncode

    if return_code != 0 and not ignore_error:
        print("\nError: Sniper process finished with non-zero exit code: {}".format(return_code), file=sys.stderr)
        # Join captured output for saving/error message
        console_output = "".join(console_output_list)
        print("Last 10 lines of output:", file=sys.stderr)
        print("".join(console_output.splitlines(True)[-10:]), file=sys.stderr) # Show tail
        raise Exception('Sniper execution failed with return code {}'.format(return_code))
    elif return_code != 0 and ignore_error:
        print("\nWarning: Sniper process finished with non-zero exit code: {} (ignored)".format(return_code))

    # Join captured output for saving
    console_output = "".join(console_output_list)

    # --- Generate CPI Stack ---
    cpistack = b'' # Default to empty bytes
    cpistack_script = os.path.join(SNIPER_BASE, 'tools/cpistack.py')
    sim_out_file = os.path.join(BENCHMARKS, 'sim.out')

    print("\n--- Attempting to generate CPI stack ---")
    if os.path.exists(cpistack_script) and os.path.exists(sim_out_file):
        try:
            # Use sys.executable to run the script with the correct Python version
            cpistack_cmd = [sys.executable, cpistack_script]
            # Capture output (stdout and stderr combined)
            cpistack = subprocess.check_output(cpistack_cmd, cwd=BENCHMARKS, stderr=subprocess.STDOUT)
            print("CPI Stack generation successful.")
        except subprocess.CalledProcessError as e:
            # CPI stack script likely failed
            print("\nError generating CPI stack (return code {}). Output:".format(e.returncode), file=sys.stderr)
            # Decode output for printing/logging
            cpi_output = e.output.decode('utf-8', 'replace')
            print(cpi_output, file=sys.stderr)
            cpistack_fail_msg = 'CPI Stack generation failed.\nReturn Code: {}\nOutput:\n{}'.format(e.returncode, cpi_output)
            cpistack = cpistack_fail_msg.encode('utf-8') # Store error message as bytes
            if not ignore_error:
                 # Don't raise here, allow saving output first, but maybe log severity
                 print("Error: CPI Stack generation failed, continuing to save output.", file=sys.stderr)
        except Exception as e:
             # Other errors (e.g., file not found if sys.executable is wrong)
             print("\nUnexpected error during CPI stack generation: {}".format(e), file=sys.stderr)
             traceback.print_exc(file=sys.stderr)
             cpistack_fail_msg = 'CPI Stack generation failed (unexpected error).\n{}'.format(e)
             cpistack = cpistack_fail_msg.encode('utf-8')
             if not ignore_error:
                 print("Error: CPI Stack generation failed, continuing to save output.", file=sys.stderr)
    else:
        # Script or sim.out missing
        missing = []
        if not os.path.exists(cpistack_script): missing.append(cpistack_script)
        if not os.path.exists(sim_out_file): missing.append(sim_out_file)
        print("Warning: Cannot generate CPI stack. Missing: {}".format(", ".join(missing)))
        cpistack = b'CPI Stack prerequisites not met.'

    ended = datetime.datetime.now()
    print("--- Run finished in {} ---".format(ended - started))

    # --- Save results ---
    save_output(base_configuration, benchmark, console_output, cpistack, started, ended)
    print("--- Output saving process complete ---")


def try_run(base_configuration, benchmark, ignore_error=False, perforation_script=None):
    """Wrapper around run() to catch and report exceptions for a single run."""
    try:
        run(base_configuration, benchmark, ignore_error=ignore_error, perforation_script=perforation_script)
        print("--- Successfully completed run: {} / {} ---".format(benchmark, '+'.join(base_configuration)))
    except KeyboardInterrupt:
        print("\nRun interrupted by user (Ctrl+C).")
        # Re-raise to allow stopping the whole script execution
        raise
    except Exception as e:
        # Catch any other exceptions from run() or its callees
        print('\n' + '#' * 80)
        print('### ERROR DURING try_run EXECUTION ###')
        print('Benchmark: {}'.format(benchmark))
        print('Configuration: {}'.format('+'.join(base_configuration)))
        # Print the full traceback
        traceback.print_exc(file=sys.stderr)
        print('#' * 80 + '\n')
        print("Attempting to continue with the next run (if any)...")
        # Optional: pause here if needed for debugging
        # input('Press Enter to continue...')


class Infeasible(Exception):
    """Custom exception for infeasible benchmark/parallelism combinations."""
    pass


# --- Benchmark Instance Generation ---

# This dictionary defines the valid thread counts for each benchmark.
# It's crucial for correctness. Update if needed based on benchmark behavior.
# 0 can indicate known failures or extreme slowness for that count.
BENCHMARK_THREAD_COUNTS = {
    # PARSEC 3.0
    'parsec-blackscholes':   [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'parsec-bodytrack':      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Needs >= 3 for larger inputs usually
    'parsec-canneal':        [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'parsec-dedup':          [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Needs >= 3 for larger inputs
    'parsec-fluidanimate':   [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'parsec-streamcluster':  [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'parsec-swaptions':      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'parsec-x264':           [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Check if high counts are stable
    # PARSEC 2.1 (if needed) - might have different counts
    # 'parsec-ferret': ...
    # 'parsec-freqmine': ...
    # 'parsec-raytrace': ...
    # 'parsec-vips': ...

    # SPLASH-2 / SPLASH-2x
    'splash2-barnes':        [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'splash2-cholesky':      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Check input size dependencies
    'splash2-fft':           [1, 2, 4, 8, 16], # Powers of 2 typical
    'splash2-fmm':           [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'splash2-lu.cont':       [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'splash2-lu.ncont':      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'splash2-ocean.cont':    [1, 2, 4, 8, 16], # Often depends on grid size being divisible
    'splash2-ocean.ncont':   [1, 2, 4, 8, 16],
    'splash2-radiosity':     [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16], # Can be slow
    'splash2-radix':         [1, 2, 4, 8, 16], # Powers of 2 typical
    'splash2-raytrace':      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'splash2-water.nsq':     [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'splash2-water.sp':      [1, 2, 4, 8, 16], # Powers of 2 typical
    # SPLASH-2x (if needed)
    # 'splash2x-barnes': ...
}

def get_instance(benchmark, parallelism, input_set='small'):
    """Generates the benchmark instance string (e.g., 'parsec-name-simsmall-p')."""
    if benchmark not in BENCHMARK_THREAD_COUNTS:
        # Use .format() for compatibility
        raise ValueError("Benchmark '{}' is not defined in BENCHMARK_THREAD_COUNTS.".format(benchmark))

    valid_ps = BENCHMARK_THREAD_COUNTS[benchmark]
    if parallelism <= 0 or parallelism not in valid_ps:
        # Use .format()
        raise Infeasible("Parallelism {} is not feasible for benchmark {}. Valid counts: {}".format(parallelism, benchmark, valid_ps))

    try:
        # The instance name needs the 1-based index 'p' corresponding to the parallelism value
        p_index = valid_ps.index(parallelism) + 1
    except ValueError:
        # Should not happen if the checks above pass, but include for safety
        raise Infeasible("Internal error: Value {} not found in list {} for benchmark {}".format(parallelism, valid_ps, benchmark))

    # Adjust input set name for PARSEC benchmarks if necessary
    processed_input_set = input_set
    if benchmark.startswith('parsec') and not input_set.startswith('sim'):
        processed_input_set = 'sim' + input_set

    # Construct the final instance name string
    instance_name = '{}-{}-{}'.format(benchmark, processed_input_set, p_index)
    # print("Generated instance name:", instance_name) # Optional debug print
    return instance_name

def get_feasible_parallelisms(benchmark):
    """Returns a sorted list of valid parallelism values for a given benchmark."""
    if benchmark not in BENCHMARK_THREAD_COUNTS:
        print("Warning: Benchmark '{}' not defined, cannot get feasible parallelisms.".format(benchmark), file=sys.stderr)
        return []
    # Return the list directly from the dictionary
    return sorted(BENCHMARK_THREAD_COUNTS[benchmark])


# --- Workload Generation (Potentially complex, use with care) ---
def get_workload(benchmark, cores, parallelism=None, number_tasks=None, input_set='small'):
    """
    Generates a list of benchmark instance strings for multi-programming scenarios.
    Tries to distribute 'cores' among 'number_tasks', potentially using 'parallelism' hints.
    NOTE: This logic can be complex and might need simplification for basic multi-program tests.
    """
    if parallelism is not None:
        # Calculate tasks based on parallelism hint
        if parallelism <= 0: raise ValueError("Parallelism must be positive")
        # Use integer division
        calculated_tasks = int(cores // parallelism) if parallelism > 0 else 0
        # print("get_workload: Hint parallelism {}, cores {}. Calculated tasks: {}".format(parallelism, cores, calculated_tasks)) # Debug
        # Call recursively with calculated number of tasks
        return get_workload(benchmark, cores, number_tasks=calculated_tasks, input_set=input_set)

    elif number_tasks is not None:
        # Base case: No more tasks needed
        if number_tasks <= 0:
            return []

        # Base case: No more cores available, but tasks still needed (infeasible)
        if cores <= 0:
            raise Infeasible("Cannot schedule {} tasks with 0 cores remaining.".format(number_tasks))

        # Calculate target parallelism per remaining task
        # Use floating point division then ceiling
        target_parallelism = int(math.ceil(cores / float(number_tasks)))
        # print("get_workload: Target parallelism {}, cores {}, tasks {}".format(target_parallelism, cores, number_tasks)) # Debug

        # Iterate downwards from the most optimistic parallelism find a feasible one
        for p_try in reversed(range(1, min(cores, target_parallelism) + 1)):
            try:
                # Check if this parallelism is valid for the benchmark
                instance = get_instance(benchmark, p_try, input_set=input_set)
                # If valid, recursively schedule remaining tasks with remaining cores
                remaining_cores = cores - p_try
                remaining_tasks = number_tasks - 1
                # print("get_workload: Trying instance {}, remaining cores {}, tasks {}".format(instance, remaining_cores, remaining_tasks)) # Debug

                # Ensure we don't recurse into negative cores/tasks
                if remaining_cores >= 0 and remaining_tasks >= 0:
                    # Make the recursive call
                    remaining_workload = get_workload(benchmark, remaining_cores, number_tasks=remaining_tasks, input_set=input_set)
                    # If recursion didn't raise Infeasible, we found a valid split for this level
                    return [instance] + remaining_workload
                else:
                    # Should not happen with correct logic, indicates issue
                    print("Warning: Inconsistent state in get_workload recursion.", file=sys.stderr)
                    continue # Try smaller parallelism

            except Infeasible:
                # This p_try is not valid for the benchmark or couldn't schedule recursively
                # print("get_workload: Parallelism {} infeasible, trying smaller.".format(p_try)) # Debug
                continue # Try the next smaller parallelism

        # If the loop completes without returning, no feasible split was found
        raise Infeasible("Could not find feasible workload split for {} cores / {} tasks.".format(cores, number_tasks))

    else:
        # Programming error: must provide either parallelism or number_tasks
        raise ValueError('Internal error: get_workload requires either parallelism or number_tasks.')


# --- Experiment Suites ---

def multi_threading_characterization():
    """Runs benchmarks required for Assignment 2, Section 3 (Multi-Threading)."""
    print("\n" + "=" * 60)
    print("=== Starting Multi-Threading Characterization ===")
    print("=" * 60)

    benchmarks_to_test = ['parsec-blackscholes', 'parsec-streamcluster']
    # Fixed config to isolate threading effects. Adjust if needed.
    # Example: High frequency, simple DVFS
    base_configuration = [
        # 'gainestown', # Add this ONLY if base.cfg has 'include = "config/gainestown.cfg"'
        'maxfreq',
        'slowdvfs',
        'pinned'      # Use the pinned scheduler
    ]
      
    if os.path.exists(os.path.join(SNIPER_BASE, 'config/gainestown.cfg')):
         if 'gainestown' not in base_configuration: base_configuration.insert(0, 'gainestown')
    
    input_set = 'simsmall' # Use small input for reasonable simulation time

    print("Using Base Configuration: {}".format('+'.join(base_configuration)))
    print("Using Input Set: {}".format(input_set))
    print("Target Core Count: {}".format(NUMBER_CORES))
    print("---")

    if ENABLE_HEARTBEATS:
        # Optional: include heartbeats if specifically needed for this analysis
        print("Heartbeats enabled, adding 'hb_enabled' to config.")
        if 'hb_enabled' not in base_configuration:
            base_configuration.append('hb_enabled')

    total_runs = 0
    successful_runs = 0

    for benchmark_name in benchmarks_to_test:
        print("\n--- Testing Benchmark: [{}] ---".format(benchmark_name))
        try:
            feasible_ps = get_feasible_parallelisms(benchmark_name)
            # Filter to only run parallelisms up to the number of physical cores
            report_ps = [p for p in feasible_ps if p <= NUMBER_CORES]

            if not report_ps:
                 print("Warning: No feasible parallelisms <= {} cores found for {}. Skipping.".format(NUMBER_CORES, benchmark_name))
                 continue

            print("Parallelisms to test (up to {} cores): {}".format(NUMBER_CORES, report_ps))

            for parallelism in report_ps:
                # Use .format()
                print("\n--- Running: [{}] / Parallelism: [{}] ---".format(benchmark_name, parallelism))
                total_runs += 1
                try:
                    # Generate the instance string (e.g., parsec-blackscholes-simsmall-4)
                    instance_string = get_instance(benchmark_name, parallelism, input_set)
                    # Execute using try_run (handles internal errors and KeyboardInterrupt)
                    # Set ignore_error=True to allow the script to continue if one run fails
                    try_run(base_configuration, instance_string, ignore_error=True)

                    # Basic check for success (presence of results directory)
                    run_name_part = 'results_{}_{}_{}'.format(BATCH_START, '_'.join(base_configuration).replace('+', '_plus_').replace('.', 'p'), instance_string.replace(',', '_'))
                    found_dir = False
                    if os.path.exists(RESULTS_FOLDER):
                        # Need to handle potential '__etc' truncation if name was long
                        prefix_to_match = run_name_part
                        if '__etc' in instance_string:
                            prefix_to_match = run_name_part.split('__etc')[0]

                        for item in os.listdir(RESULTS_FOLDER):
                             # Check if the directory name starts with the expected prefix
                             if item.startswith(prefix_to_match) and os.path.isdir(os.path.join(RESULTS_FOLDER, item)):
                                 found_dir = True
                                 break
                    if found_dir:
                        successful_runs += 1
                        print("--- Verified results directory found for this run ---")
                    else:
                        print("--- Warning: Results directory may not have been created for this run ---", file=sys.stderr)


                except Infeasible as e:
                    # Should not happen if get_feasible_parallelisms worked correctly
                    print("Error: Infeasible combination: {} / {} - {}".format(benchmark_name, parallelism, e), file=sys.stderr)
                # Other exceptions are caught by try_run

        except ValueError as e:
             # Error in get_feasible_parallelisms (e.g., benchmark name typo)
             print("Error setting up benchmark {}: {}. Skipping.".format(benchmark_name, e), file=sys.stderr)
        except Exception as e:
            # Catch any other unexpected error during the outer loop
            print("Unexpected error during setup/loop for benchmark {}:".format(benchmark_name), file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            print("Attempting to continue with the next benchmark...")

    print("\n" + "=" * 60)
    print("=== Multi-Threading Characterization Complete ===")
    print("Total runs attempted: {}".format(total_runs))
    print("Successful runs (results dir check): {}".format(successful_runs))
    print("(Note: Success check only verifies directory creation, not simulation correctness.)")
    print("=" * 60 + "\n")


# --- Other Demo/Test Functions (Updated for Python 3) ---

def example():
    """Original example function, updated for Python 3."""
    print("\n=== Running Original Example ===")
    # Reduced set for quicker example run
    for benchmark in ('parsec-blackscholes', 'parsec-streamcluster'):
        feasible_ps = get_feasible_parallelisms(benchmark)
        if not feasible_ps:
            print("Skipping {} - no feasible parallelism found.".format(benchmark))
            continue
        # Example: run with 1.0GHz and 4.0GHz, using 2 threads if possible
        for freq in (1.0, 4.0):
            parallelism = 2
            if parallelism in feasible_ps:
                 instance = get_instance(benchmark, parallelism, input_set='simsmall')
                 # Use a simple base config for example
                 try_run(['{:.1f}GHz'.format(freq), 'maxFreq', 'slowDVFS'], instance, ignore_error=True)
            else:
                 print("Skipping infeasible {} / {} threads".format(benchmark, parallelism))
    print("=== Original Example Complete ===\n")

def example_symmetric_perforation():
    """Symmetric perforation example, updated for Python 3."""
    print("\n=== Running Symmetric Perforation Example ===")
    for benchmark in ('parsec-blackscholes',):
        feasible_ps = get_feasible_parallelisms(benchmark)
        if not feasible_ps: continue
        perforation_rate = "50" # Perforation rate as string
        freq = 4.0
        parallelism = 4
        if parallelism in feasible_ps:
            instance = get_instance(benchmark, parallelism, input_set='simsmall')
            # Format script argument string
            script = "magic_perforation_rate:{}".format(perforation_rate)
            try_run(['{:.1f}GHz'.format(freq), 'maxFreq', 'slowDVFS'], instance,
                    ignore_error=True, perforation_script=script)
        else:
             print("Skipping infeasible {} / {}".format(benchmark, parallelism))
    print("=== Symmetric Perforation Example Complete ===\n")

def example_asymmetric_perforation():
     """Asymmetric perforation example, updated for Python 3."""
     print("\n=== Running Asymmetric Perforation Example ===")
     # Define benchmarks and their *assumed* number of main loops for perforation
     bench_loops = [("parsec-blackscholes", 1), ("parsec-streamcluster", 2)] # Example values
     for benchmark, num_loops in bench_loops:
         feasible_ps = get_feasible_parallelisms(benchmark)
         if not feasible_ps: continue
         # Create comma-separated rates, e.g., "0,10" if num_loops=2
         loop_rates = ','.join([str(i*10) for i in range(num_loops)])
         freq = 4.0
         parallelism = 4
         if parallelism in feasible_ps:
             instance = get_instance(benchmark, parallelism, input_set='simsmall')
             script = 'magic_perforation_rate:{}'.format(loop_rates)
             try_run(['{:.1f}GHz'.format(freq), 'maxFreq', 'slowDVFS'], instance,
                      ignore_error=True, perforation_script=script)
         else:
             print("Skipping infeasible {} / {}".format(benchmark, parallelism))
     print("=== Asymmetric Perforation Example Complete ===\n")


def multi_program():
    """Multi-program example, updated for Python 3."""
    print("\n=== Running Multi-Program Example ===")
    input_set = 'simsmall'
    # Base config might need 'open' scheduler enabled via config/base.cfg
    base_configuration = ['4.0GHz', "maxFreq", 'slowDVFS', 'open'] # Add 'open' scheduler config
    # Example: Run blackscholes (1 thread) and streamcluster (1 thread) together
    benchmark_set = [
        ('parsec-blackscholes', 1),
        ('parsec-streamcluster', 1)
    ]

    if ENABLE_HEARTBEATS:
        if 'hb_enabled' not in base_configuration: base_configuration.append('hb_enabled')

    benchmarks_list = []
    total_cores_needed = 0
    for bench_name, para in benchmark_set:
         try:
             benchmarks_list.append(get_instance(bench_name, para, input_set))
             total_cores_needed += para
         except (Infeasible, ValueError) as e:
             print("Cannot create instance for multi-program: {} (para {}) - {}".format(bench_name, para, e))
             return # Abort if any instance fails

    if total_cores_needed > NUMBER_CORES:
        print("Warning: Multi-program workload requires {} cores, but only {} available.".format(total_cores_needed, NUMBER_CORES), file=sys.stderr)
        # Decide whether to proceed or abort if oversubscribing is not desired
        # return

    benchmarks_string = ','.join(benchmarks_list)
    print("Running multi-program workload: {}".format(benchmarks_string))
    # Ensure arrivalRate is set appropriately in config/base.cfg or via sniper args if possible
    # e.g., add '-s scheduler/open/arrivalRate=2' to args if needed and supported
    try_run(base_configuration, benchmarks_string, ignore_error=True)
    print("=== Multi-Program Example Complete ===\n")

def test_static_power():
    """Static power test, updated for Python 3."""
    print("\n=== Testing Static Power ===")
    try:
        # Run with 1 thread usually sufficient
        instance = get_instance('parsec-blackscholes', 1, input_set='simsmall')
        # Ensure 'testStaticPower' is a valid config included in base.cfg
        try_run(['4.0GHz', 'testStaticPower', 'slowDVFS'], instance, ignore_error=True)
    except (Infeasible, ValueError) as e:
        print("Failed to get instance for static power test: {}".format(e))
    print("=== Static Power Test Complete ===\n")

def ondemand_demo():
    """OnDemand governor demo, updated for Python 3."""
    print("\n=== Running On-Demand Governor Demo ===")
    try:
        # Use max cores to demonstrate scaling behavior
        instance = get_instance('parsec-blackscholes', NUMBER_CORES, input_set='simsmall')
        # Ensure 'ondemand' and 'fastDVFS' are valid configs included in base.cfg
        try_run(['4.0GHz', 'ondemand', 'fastDVFS'], instance, ignore_error=True)
    except (Infeasible, ValueError) as e:
        print("Failed to get instance for ondemand demo (need {} cores): {}".format(NUMBER_CORES, e), file=sys.stderr)
    print("=== On-Demand Demo Complete ===\n")

def coldestcore_demo():
     """ColdestCore governor demo, updated for Python 3."""
     print("\n=== Running Coldest Core Demo ===")
     try:
        instance = get_instance('parsec-blackscholes', NUMBER_CORES, input_set='simsmall')
        # Lower base frequency might show effect better
        # Ensure 'coldestCore' is a valid config included in base.cfg
        try_run(['2.4GHz', 'maxFreq', 'slowDVFS', 'coldestCore'], instance, ignore_error=True)
     except (Infeasible, ValueError) as e:
        print("Failed to get instance for coldestcore demo (need {} cores): {}".format(NUMBER_CORES, e), file=sys.stderr)
     print("=== Coldest Core Demo Complete ===\n")


# --- Main Execution ---

def main():
    """Main function to select and run experiment suites."""
    print("Starting EEEC Simulation Control Script")
    print("Timestamp: {}".format(BATCH_START))
    print("Python Version: {}".format(platform.python_version()))
    print("Sniper Base: {}".format(SNIPER_BASE))
    print("Benchmarks Dir: {}".format(BENCHMARKS))
    print("Results Dir: {}".format(RESULTS_FOLDER))
    print("-" * 30)

    # --- Select which test suite(s) to run ---

    # *** Assignment 2 - Section 3: Multi-threading Characterization ***
    multi_threading_characterization()

    # --- Other examples/demos (uncomment to run specific ones) ---
    # print("\nRunning additional demos/examples...")
    # example()
    # ondemand_demo()
    # coldestcore_demo()
    # test_static_power()
    # multi_program()
    # example_symmetric_perforation()
    # example_asymmetric_perforation()
    # print("Finished additional demos/examples.")

if __name__ == '__main__':
    # Check Python version >= 3.5 at runtime start
    if sys.version_info[0] < 3 or (sys.version_info[0] == 3 and sys.version_info[1] < 5):
         print("Error: This script requires Python 3.5 or newer. You are using {}. Aborting.".format(platform.python_version()), file=sys.stderr)
         sys.exit(1) # Exit if version is too old

    # Ensure results folder exists before starting any runs
    if not os.path.exists(RESULTS_FOLDER):
        try:
            os.makedirs(RESULTS_FOLDER)
            print("Created results folder: {}".format(RESULTS_FOLDER))
        except OSError as e:
            print("Fatal Error: Could not create results folder {}: {}. Aborting.".format(RESULTS_FOLDER, e), file=sys.stderr)
            sys.exit(1)

    # Start the main execution logic
    try:
        main()
        print("\nScript finished successfully.")
    except KeyboardInterrupt:
        print("\nScript interrupted by user (Ctrl+C). Exiting.")
        sys.exit(1)
    except Exception as e:
        print("\nAn unexpected error occurred during main execution:", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)