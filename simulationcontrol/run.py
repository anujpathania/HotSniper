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
            os.remove(file_path)

    for f in os.listdir(BENCHMARKS):
        if ('output.' in f) or ('.264' in f) or ('poses.' in f) or ('app_mapping' in f) :
            os.remove(os.path.join(BENCHMARKS, f))
        

def save_output(base_configuration, benchmark, console_output, cpistack, started, ended, label: str):
    benchmark_text = benchmark
    if len(benchmark_text) > 100:
        benchmark_text = benchmark_text[:100] + '__etc'
    run = 'results_{}_{}_{}_{}'.format(BATCH_START, '+'.join(base_configuration), benchmark_text, label)
    directory = os.path.join(RESULTS_FOLDER, run)
    if not os.path.exists(directory):
        os.makedirs(directory)
    with gzip.open(os.path.join(directory, 'execution.log.gz'), 'w') as f:
        f.write(console_output.encode('utf-8'))
    with open(os.path.join(directory, 'executioninfo.txt'), 'w') as f:
        f.write('started:    {}\n'.format(started.strftime('%Y-%m-%d %H:%M:%S')))
        f.write('ended:      {}\n'.format(ended.strftime('%Y-%m-%d %H:%M:%S')))
        f.write('duration:   {}\n'.format(ended - started))
        f.write('host:       {}\n'.format(platform.node()))
        f.write('tasks:      {}\n'.format(benchmark))
    with open(os.path.join(directory, 'cpi-stack.txt'), 'wb') as f:
        f.write(cpistack)
    for f in ('sim.cfg',
              'sim.info',
              'sim.out',
              'cpi-stack.png',
              'sim.stats.sqlite3'):
        shutil.copy(os.path.join(BENCHMARKS, f), directory)
    for f in ('PeriodicPower.log',
              'PeriodicThermal.log',
              'PeriodicFrequency.log',
              'PeriodicVdd.log',
              'PeriodicCPIStack.log',
              'PeriodicRvalue.log'):
        with open(os.path.join(BENCHMARKS, f), 'rb') as f_in, gzip.open('{}.gz'.format(os.path.join(directory, f)), 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

    pattern = r"^\d+\.hb.log$" # Heartbeat logs
    for f in os.listdir(BENCHMARKS):
        if not re.match(pattern, f):
            continue
        shutil.copy(os.path.join(BENCHMARKS, f), directory)
    
    for f in os.listdir(BENCHMARKS):
        if 'output.' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)
        elif 'poses.' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)
        elif '.264' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)
        elif 'app_mapping.' in f:
            shutil.copy(os.path.join(BENCHMARKS, f), directory)

    create_plots(run)


def run(base_configuration, benchmark, label: str, script: str, ignore_error=False):
    print('running {}: {} with configuration {}'.format(label, benchmark, '+'.join(base_configuration)))
    started = datetime.datetime.now()
    change_base_configuration(base_configuration)

    prev_run_cleanup()

    benchmark_options = []
    if ENABLE_HEARTBEATS == True:
        benchmark_options.append('enable_heartbeats')
        benchmark_options.append('hb_results_dir=%s' % BENCHMARKS)

    # NOTE: This determines the logging interval! (see issue in forked repo)
    periodicPower = 1000000
    #periodicPower = 250000
    if 'mediumDVFS' in base_configuration:
        periodicPower = 250000
    if 'fastDVFS' in base_configuration:
        periodicPower = 100000 
   
    args = '-n {number_cores} -c {config} --benchmarks={benchmark} --no-roi --sim-end=last -senergystats:{periodic} -speriodic-power:{periodic}{script}{benchmark_options}' \
        .format(number_cores=NUMBER_CORES,
                config=SNIPER_CONFIG,
                benchmark=benchmark,
                periodic=periodicPower,
                script= ' -s'+script + ''.join([' -s' + s for s in SCRIPTS]),
                benchmark_options=''.join([' -B ' + opt for opt in benchmark_options]))
    
    console_output = ''

    print(args)

    run_sniper = os.path.join(BENCHMARKS, 'run-sniper')
    p = subprocess.Popen([run_sniper] + args.split(' '), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=1, cwd=BENCHMARKS)
    with p.stdout:
        for line in iter(p.stdout.readline, b''):
            linestr = line.decode('utf-8')
            console_output += linestr
            print(linestr, end='')

    p.wait()

    try:
        cpistack = subprocess.check_output(['python', os.path.join(SNIPER_BASE, 'tools/cpistack.py')], cwd=BENCHMARKS)
    except:
        if ignore_error:
            cpistack = b''
        else:
            raise

    ended = datetime.datetime.now()

    save_output(base_configuration, benchmark, console_output, cpistack, started, ended, label)

    if p.returncode != 0:
        raise Exception('return code != 0')


def try_run(base_configuration, benchmark, ignore_error=False):
    try:
        run(base_configuration, benchmark, ignore_error=ignore_error)
    except KeyboardInterrupt:
        raise
    except Exception as e:
        for i in range(4):
            print('#' * 80)
        #print(e)
        print(traceback.format_exc())
        for i in range(4):
            print('#' * 80)
        input('Please press enter...')


class Infeasible(Exception):
    pass


def get_instance(benchmark, parallelism, input_set='small'):
    threads = {
        'parsec-blackscholes': [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-bodytrack': [3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-canneal': [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-dedup': [4, 7, 10, 13, 16],
        'parsec-fluidanimate': [2, 3, 0, 5, 0, 0, 0, 9],
        'parsec-streamcluster': [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-swaptions': [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'parsec-x264': [1, 3, 4, 5, 6, 7, 8, 9],
        'splash2-barnes': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-cholesky': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-fft': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16],
        'splash2-fmm': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-lu.cont': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-lu.ncont': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-ocean.cont': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16],
        'splash2-ocean.ncont': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16],
        'splash2-radiosity': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-radix': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16],
        'splash2-raytrace': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-water.nsq': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
        'splash2-water.sp': [1, 2, 0, 4, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 16],  # other parallelism values run but are suboptimal -> don't allow in the first place
    }
    
    ps = threads[benchmark]
    if parallelism <= 0 or parallelism not in ps:
        raise Infeasible()
    p = ps.index(parallelism) + 1

    if benchmark.startswith('parsec') and not input_set.startswith('sim'):
        input_set = 'sim' + input_set

    return '{}-{}-{}'.format(benchmark, input_set, p)


def get_feasible_parallelisms(benchmark):
    feasible = []
    for p in range(1, 16+1):
        try:
            get_instance(benchmark, p)
            feasible.append(p)
        except Infeasible:
            pass
    return feasible


def get_workload(benchmark, cores, parallelism=None, number_tasks=None, input_set='small'):
    if parallelism is not None:
        number_tasks = math.floor(cores / parallelism)
        return get_workload(benchmark, cores, number_tasks=number_tasks, input_set=input_set)
    elif number_tasks is not None:
        if number_tasks == 0:
            if cores == 0:
                return []
            else:
                raise Infeasible()
        else:
            parallelism = math.ceil(cores / number_tasks)
            for p in reversed(range(1, min(cores, parallelism) + 1)):
                try:
                    b = get_instance(benchmark, p, input_set=input_set)
                    return [b] + get_workload(benchmark, cores - p, number_tasks=number_tasks-1, input_set=input_set)
                except Infeasible:
                    pass
            raise Infeasible()
    else:
        raise Exception('either parallelism or number_tasks needs to be set')


def dev_single():
    benchmark = ('parsec-canneal', 3)

    freq = 4
    parallelism = 4

    pr = 50
    pr_vec = [str(pr) for e in range(benchmark[1])]

    run(label="development_pr:{}".format(','.join(pr_vec)), 
        base_configuration=['{:.1f}GHz'.format(freq), 'maxFreq'], # 'slowDVFS' 
        benchmark=get_instance(benchmark[0], parallelism, input_set='small'),
        script='magic_perforation_rate:%s' % ','.join(pr_vec))


def single_program_perforation_rate_background_mask():
    for benchmark in (
                        ('parsec-swaptions', 3),
                        # ('parsec-bodytrack', 6),
                        # ('parsec-x264', 6),
                        # ('parsec-streamcluster', 2),
                        # ('parsec-blackscholes', 1),
                        # ('parsec-canneal', 3), 
                    ):
        for background_pr in (0, 25, 50, 75,):
            for loop_pr in (0, 25, 50, 75,):
                for loop in range(benchmark[1]):
                    if(background_pr == loop_pr and loop <= 1):
                        continue;

                    freq = 4
                    parallelism = 3

                    pr_vec = [str(background_pr) for e in range(benchmark[1])]
                    pr_vec[loop] = str(loop_pr)

                    run(label="model_collection:{}".format(','.join(pr_vec)), 
                        base_configuration=['{:.1f}GHz'.format(freq), 'maxFreq'], # 'slowDVFS' 
                        benchmark=get_instance(benchmark[0], parallelism, input_set='small'),
                        script='magic_perforation_rate:%s' % ','.join(pr_vec))
                    
def perforation_rate_loop_pair_profile(loop_a: int, loop_b: int, benchmark_loop):
    freq = 4
    parallelism = 3
    background_pr = 0
    
    pr_vec = [str(background_pr) for e in range(benchmark_loop[1])]
    
    for pr_a in (0, 20, 40, 60):
        for pr_b in (0, 20, 40, 60):

            pr_vec[loop_a] = str(pr_a)
            pr_vec[loop_b] = str(pr_b)

            run(label="swaptions_surface_3_a{}_b{}:{}".format(loop_a, loop_b, ','.join(pr_vec)), 
                base_configuration=['{:.1f}GHz'.format(freq), 'maxFreq'], # 'slowDVFS' 
                benchmark=get_instance(benchmark_loop[0], parallelism, input_set='small'),
                script='magic_perforation_rate:%s' % ','.join(pr_vec))

def perforation_rate_loop_pair_profile(loop_a: int, loop_b: int, benchmark_loop):
    freq = 4
    parallelism = 3
    background_pr = 0
    
    pr_vec = [str(background_pr) for e in range(benchmark_loop[1])]
    
    for pr_a in (0, 20, 40, 60):
        for pr_b in (0, 20, 40, 60):

            pr_vec[loop_a] = str(pr_a)
            pr_vec[loop_b] = str(pr_b)

            run(label="swaptions_surface_3_a{}_b{}:{}".format(loop_a, loop_b, ','.join(pr_vec)), 
                base_configuration=['{:.1f}GHz'.format(freq), 'maxFreq'], # 'slowDVFS' 
                benchmark=get_instance(benchmark_loop[0], parallelism, input_set='small'),
                script='magic_perforation_rate:%s' % ','.join(pr_vec))
            

def perforation_rate_profile(benchmark_loop, loop_rates, background_rates):
    freq = 4
    parallelism = 3
    
    for loop in range(benchmark_loop[1]):
        for background_pr in loop_rates: #(0, 20, 40, 60):
            for loop_pr in background_rates: #(0, 18, 20, 22, 38, 40, 42, 58, 60, 62):
                pr_vec = [str(background_pr) for e in range(benchmark_loop[1])]
                pr_vec[loop] = loop_pr

                run(label="{}_surface:{}".format(benchmark_loop[0], ','.join(pr_vec)), 
                    base_configuration=['{:.1f}GHz'.format(freq), 'maxFreq'], # 'slowDVFS' 
                    benchmark=get_instance(benchmark_loop[0], parallelism, input_set='small'),
                    script='magic_perforation_rate:%s' % ','.join(pr_vec))
    


def multi_program_perforation_rate():
    input_set = 'small'

    freqency = 4
    parallel = 3

    benchmarks  = ''
    for i, benchmark in enumerate((# 'parsec-blackscholes',
                        # 'parsec-bodytrack',
                        # 'parsec-canneal', 
                        # 'parsec-streamcluster',
                        'parsec-swaptions',
                        'parsec-x264',                   
                        #   'parsec-ferret' # unimplemented
                        )):
        min_parallelism = get_feasible_parallelisms(benchmark)[0]
        
        if i != 0:
            benchmarks = benchmarks + ',' + get_instance(benchmark, parallel, input_set)
        else:
            benchmarks = benchmarks + get_instance(benchmark, parallel, input_set)

    run(label="dev_multi_prog", 
        base_configuration=['{:.1f}GHz'.format(freqency), 'maxFreq', 'slowDVFS'],
        benchmark=benchmarks)


def example():
    for benchmark in (
                    #   'parsec-blackscholes',
                    #   'parsec-bodytrack',
                    #   'parsec-canneal',
                    #   'parsec-streamcluster',
                    #   'parsec-swaptions',
                    #   'parsec-x264',
                      #'parsec-ferret'
        
                      #'parsec-fluidanimate',
                      #'parsec-dedup',
                      
                      #'splash2-barnes',
                      #'splash2-fmm',
                      #'splash2-ocean.cont',
                      #'splash2-ocean.ncont',
                      #'splash2-radiosity',
                      #'splash2-raytrace',
                      #'splash2-water.nsq',
                      #'splash2-water.sp',
                      #'splash2-cholesky',
                      #'splash2-fft',
                      #'splash2-lu.cont',
                      #'splash2-lu.ncont',
                      #'splash2-radix',
                      ):

        min_parallelism = get_feasible_parallelisms(benchmark)[0]
        max_parallelism = get_feasible_parallelisms(benchmark)[-1]
        for freq in (4, ): # SP: only 4ghz
            #for parallelism in (max_parallelism,):
            for parallelism in (4,):
                # you can also use try_run instead
                run(['{:.1f}GHz'.format(freq), 'maxFreq', 'slowDVFS'], get_instance(benchmark, parallelism, input_set='small'))


def multi_program():
    # In this example, two instances of blackscholes will be scheduled.
    # By setting the scheduler/open/arrivalRate base.cfg parameter to 2, the
    # tasks can be set to arrive at the same time.

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
        min_parallelism = get_feasible_parallelisms(benchmark)[0]
        if i != 0:
            benchmarks = benchmarks + ',' + get_instance(benchmark, min_parallelism, input_set)
        else:
            benchmarks = benchmarks + get_instance(benchmark, min_parallelism, input_set)

    run(base_configuration, benchmarks)

    
def test_static_power():
    run(['4.0GHz', 'testStaticPower', 'slowDVFS'], get_instance('parsec-blackscholes', 3, input_set='small'))


def main():
    # perforation_rate_loop_pair_profile(0, 2, ('parsec-swaptions', 3))
    # perforation_rate_loop_pair_profile(0, 1, ('parsec-swaptions', 3))
    
    perforation_rate_profile(("parsec-swaptions", 2), background_rates=(20,40,60), loop_rates=(18, 20, 22))
    perforation_rate_profile(("parsec-swaptions", 2), background_rates=(20,40,60), loop_rates=(38, 40, 42))
    perforation_rate_profile(("parsec-swaptions", 2), background_rates=(20,40,60), loop_rates=(58, 60, 62))

    # multi_program_perforation_rate()
    # dev_single()
    # single_program_perforation_rate()
    
    # example()
    # test_static_power()
    # multi_program()

if __name__ == '__main__':
    main()
