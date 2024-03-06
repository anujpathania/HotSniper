import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.axes import Axes
import numpy as np
import re
import os.path
import io
import gzip
import seaborn as sns
import cv2

from pprint import pprint
from pathlib import PurePath


RESULTS_DIR = os.path.join(os.getenv('GRAPHITE_ROOT'), 'results')

class ExpData:
    def __init__(self):
        self.idx: list[int] = []
        self.output_files: list[str] = []
        self.log_files: list[str] = []
        self.hb_df = pd.DataFrame()

def compile_testdata(label: str):
    benchmarks = {'blackscholes': [], 
                   'bodytrack': [],
                   'canneal': [],
                   'streamcluster': [], 
                   'swaptions': [], 
                   'x264': [] }
    
    for dirname in sorted(os.listdir(RESULTS_DIR)):
        if label in dirname:
            if 'blackscholes' in dirname:
                benchmarks["blackscholes"].append(dirname)
            elif 'bodytrack' in dirname:
                benchmarks['bodytrack'].append(dirname)
            elif 'canneal' in dirname:
                benchmarks['canneal'].append(dirname)
            elif 'streamcluster' in dirname:
                benchmarks['streamcluster'].append(dirname)
            elif 'swaptions' in dirname:
                benchmarks['swaptions'].append(dirname)
            elif 'x264' in dirname:
                benchmarks['x264'].append(dirname)



    benchmark_analysis = {'blackscholes': ExpData(), 
                          'bodytrack': ExpData(),
                          'canneal': ExpData(),
                          'streamcluster': ExpData(), 
                          'swaptions': ExpData(), 
                          'x264': ExpData(), }
    
    for benchmark, dirs in benchmarks.items():
        for dir in dirs:
            res = os.path.join(RESULTS_DIR, dir)
            bench: ExpData = benchmark_analysis[benchmark]

            dir_ints = [int(num) for num in re.findall(r'\d+', dir)] # getting the index for the experiment it is part of the label
            idx = dir_ints[-1]

            # set index of this run.
            bench.idx.append(idx)
            
            # save reference to output file and log_file
            for file in os.listdir(res):
                if 'output.txt' in file and benchmark in {'blackscholes', 'streamcluster'}:
                    bench.output_files.append(os.path.join(RESULTS_DIR, dir, file))

                if 'poses.txt' in file and benchmark in {'bodytrack'}:
                    bench.output_files.append(os.path.join(RESULTS_DIR, dir, file))

                if 'execution.log.gz' in file and benchmark in {'canneal', 'swaptions'}:
                    bench.output_files.append(os.path.join(RESULTS_DIR, dir, file))

                if '.264' in file and benchmark in {'x264'}:
                    bench.output_files.append(os.path.join(RESULTS_DIR, dir, file))

                if 'execution.log.gz' in file:
                    bench.log_files.append(os.path.join(RESULTS_DIR, dir, file))
                    
                if 'hb.log' in file:
                    file_df = pd.read_csv(os.path.join(RESULTS_DIR, dir, file), sep='\t')
                    file_df["X"] = idx

                    bench.hb_df = pd.concat([bench.hb_df, file_df])

    return benchmark_analysis        


def perforation_resp_time_speedup_plot(name:str, benchmark: ExpData, ax: Axes):
    responce_times = []
    x = []
    
    # get execution times.
    for i, pr in enumerate(benchmark.idx):
        execution_log =  io.TextIOWrapper(gzip.open(benchmark.log_files[i], 'r'), encoding="utf-8")

        x.append(pr)
        
        for line in execution_log:
            m = re.search(r'Task (\d+) \(Response/Service/Wait\) Time \(ns\)\s+:\s+(\d+)\s+(\d+)\s+(\d+)', line)
            if m is not None:
                responce_times.append(int(m.group(2)))
    
    # make speedup for response_times.
    speedup = [responce_times[0] / time for time in responce_times]    

    # plot the figures.
    sns.lineplot(y=speedup, x=x, ax=ax)
    ax.set_title(name + " speed-up on simsmall")
    ax.set_ylabel("speed-up")
    ax.set_xlabel("perforation rate")

def perforation_hb_time_speedup_plot(benchmark:str, data: ExpData, ax: Axes):
    responce_times = []
    X = []

    print(benchmark)
    print(data.hb_df)
    
    # get execution times.
    for x in data.hb_df['X'].unique():
        X.append(x)

        df = data.hb_df[data.hb_df['X'] == x]
        responce_times.append(df['Timestamp'].max() - df['Timestamp'].min()) 
    
    # make speedup for response_times.
    speedup = [responce_times[0] / time for time in responce_times]    

    # plot the figures.
    sns.lineplot(y=speedup, x=X, ax=ax)
    ax.set_title(benchmark + " speed-up within the heartbeat window")
    ax.set_ylabel("speed-up")
    ax.set_xlabel("perforation rate")

# TODO: get heartrate>?
def perforation_heart_rate_plot(benchmark:str, data: ExpData, ax: Axes):
    heart_rates = []
    X = []
    
    # get execution times.
    for x in data.hb_df['X'].unique():
        X.append(x)
        df = data.hb_df[data.hb_df["X"] == x]
        df = df.drop(0)
        df = df.drop(1)
        heart_rates.append(df["Instant Rate"].sum()/df["Instant Rate"].count())
        # heart_rates.append(df["Window Rate"].iloc[-1])

    sns.lineplot(y=heart_rates, x=X, ax=ax)
    ax.set_title(benchmark + "  average heart rate")
    ax.set_ylabel("beats per nanosecond")
    ax.set_xlabel("perforation rate")

# TODO: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaah
def perforation_qos_loss_plot(benchmark:str, data: ExpData, ax: Axes):
    perforation_noise = []

    for _ in data.idx:
        perforation_noise.append(0.0)
    
    output_comparison = []
        
    if benchmark in {'blackscholes', 'bodytrack'}: # text files with the outputs on lines
        for file in data.output_files:
            run_out = []
            f = open(file)
            for line in f.readlines():
                run_out += [float(num) for num in  re.findall(r'-?\b\d+\.?\d*\b', line)]
            
            output_comparison.append(run_out)
    
    elif benchmark in {'canneal'}: # one number in the execution log.
        for file in data.output_files: 
            execution_log =  io.TextIOWrapper(gzip.open(file, 'r'), encoding="utf-8")
            
            for line in execution_log:
                m = re.search(r'Final routing is: (\d+\.\d+)', line)
                if m is not None:
                    output_comparison.append([float(m.group(1))])
                    break
    
    elif benchmark in {'swaptions'}: # one number in the execution log.
        for file in data.output_files: 
            execution_log =  io.TextIOWrapper(gzip.open(file, 'r'), encoding="utf-8")
            
            run_out = []
            for line in execution_log:
                m = re.search(r'SwaptionPrice: (\d+\.\d+) StdError: (\d+\.\d+)', line)
                if m is not None:
                    run_out.append(float(m.group(1)))
                    run_out.append(float(m.group(2)))
                
            output_comparison.append(run_out)

    elif benchmark in {'x264'}: # decompose video into frames and then calculate the noise.
        # make reference.
        
        for file in data.output_files: 
            encoded_vid = cv2.VideoCapture(file)
            
            succ, img = encoded_vid.read()
            count = 0
            while succ:
                cv2.imwrite(os.path.join(RESULTS_DIR, "image_{}.jpg".format(count)), img)
                succ, img = encoded_vid.read()
                count += 1
            # 


        return
    else:
        ax.set_title(name + " QoS on simsmall")
        ax.set_ylabel("% noise")
        ax.set_xlabel("perforation rate")
        print(benchmark)
        return
    
    reference = output_comparison[0]
    for index, output in enumerate(output_comparison):
        perforation_noise[index] = 100 * (abs(sum(reference)- sum(output)) / abs(sum(reference)))


    print(perforation_noise)
    sns.lineplot(y=perforation_noise, x=data.idx, ax=ax)
    ax.set_title(name + " QoS on simsmall")
    ax.set_ylabel("% noise")
    ax.set_xlabel("perforation rate")


benchmarks = compile_testdata("_range_medium")

# Create figure and subplots
fig, axs = plt.subplots(2, 3, figsize=(15, 10))

flat_ax = (axs.flatten())

i = 0
for name, data in benchmarks.items():
    print(name)
    perforation_qos_loss_plot(name, data, flat_ax[i])
    i+=1

plt.tight_layout()
plt.show()


